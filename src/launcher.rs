use std::env::current_exe;
use std::process::Command;

use anyhow::Result;

use crate::bootstrapper;

struct Args {
    _launch_mode: String,
    client_version: String,
    game_info: String,
    place_launcher_url: String,
}

pub async fn launch(uri: &str) -> Result<()> {
    let (up_to_date, latest_version) = bootstrapper::is_up_to_update().await?;
    if !up_to_date {
        paris::info!("Out ouf date, updating");
        bootstrapper::bootstrap().await?;
    }
    if !uri.starts_with("pekora2-player:") {
        anyhow::bail!("Invalid URI");
    }
    let re = regex::Regex::new(r"pekora2-player:1\+launchmode:([^+]+)\+clientversion:([^+]+)\+gameinfo:([^+]+)\+placelauncherurl:([^+]+)").unwrap();
    let captures = re
        .captures(uri)
        .ok_or_else(|| anyhow::anyhow!("Invalid URI format"))?;

    let args = Args {
        _launch_mode: captures.get(1).unwrap().as_str().to_string(),
        client_version: captures.get(2).unwrap().as_str().to_string(),
        game_info: captures.get(3).unwrap().as_str().to_string(),
        place_launcher_url: captures.get(4).unwrap().as_str().to_string(),
    };
    paris::info!("Starting {}", args.client_version);
    let exe_path = current_exe()?;
    let install_path = exe_path.parent().unwrap();
    Command::new(
        install_path
            .join("Versions")
            .join(&latest_version)
            .join(&args.client_version)
            .join("ProjectXPlayerBeta.exe"),
    )
    .arg("--play")
    .arg("-a")
    .arg("https://www.pekora.org/Login/Negotiate.ashx")
    .arg("-t")
    .arg(args.game_info)
    .arg("-j")
    .arg(args.place_launcher_url)
    .spawn()?;
    paris::success!("Started Pekora");
    Ok(())
}
