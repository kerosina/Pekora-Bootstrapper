#![warn(clippy::pedantic)]
use std::{
    env,
    io::{self, Write},
    thread,
    time::Duration,
};

use figlet_rs::FIGfont;
use termcolor::{Color, ColorChoice, ColorSpec, StandardStream, WriteColor};

mod bootstrapper;
mod config;
mod launcher;
mod utils;

#[tokio::main]
async fn main() {
    let font = FIGfont::from_content(include_str!("../assets/alligator.flf")).unwrap();
    let figlet_text = font.convert("Pekora").unwrap().to_string();
    let mut stdout = StandardStream::stdout(ColorChoice::Auto);
    stdout
        .set_color(ColorSpec::new().set_fg(Some(Color::Blue)))
        .unwrap();
    write!(&mut stdout, "{figlet_text}").unwrap();
    stdout.reset().unwrap();
    println!();
    stdout
        .set_color(ColorSpec::new().set_fg(Some(Color::Blue)).set_dimmed(true))
        .unwrap();
    write!(&mut stdout, "pekora.rip").unwrap();
    stdout.reset().unwrap();
    println!();

    let mut args = env::args();
    let _ = args.next();
    match args.next() {
        None => {
            if let Err(err) = bootstrapper::bootstrap().await {
                paris::error!("Error while bootstrapping: {err:?}");
            }
            println!("Press enter to exit");
            let _ = io::stdin().read_line(&mut String::new());
        }
        Some(x) if x.starts_with("pekora2-player") => {
            if let Err(err) = launcher::launch(&x).await {
                paris::error!("Error while launching: {err:?}");
                println!("Press enter to exit");
                let _ = io::stdin().read_line(&mut String::new());
            } else {
                println!("Closing in 5 seconds");
                thread::sleep(Duration::from_secs(5));
            }
        }
        _ => {
            paris::error!("Unknown argument(s). Pass 0 arguments to install or update Pekora.");
            thread::sleep(Duration::from_secs(5));
        }
    }
}
