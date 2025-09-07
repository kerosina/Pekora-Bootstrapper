use std::{
    env::{self, current_exe},
    fs::{self, File},
    io::{self, Cursor},
    path::PathBuf,
};

use crate::{
    config::{POST_INSTALL_URL, SETUP},
    utils,
};
use anyhow::Result;
use anyhow::anyhow;
use reqwest::Client;
use zip::ZipArchive;

pub async fn is_up_to_update() -> Result<(bool, String)> {
    let client = Client::new();
    let latest_version = client
        .get(format!("{SETUP}/version"))
        .send()
        .await?
        .text()
        .await?;
    let local_appdata = env::var("LOCALAPPDATA")?;
    let install_dir = PathBuf::from(local_appdata).join("Pekora2");
    if fs::read_to_string(install_dir.join("version"))
        .map(|x| x == latest_version)
        .unwrap_or(false)
    {
        Ok((true, latest_version))
    } else {
        Ok((false, latest_version))
    }
}

#[allow(clippy::too_many_lines, reason = "code is more readable as it is")]
pub async fn bootstrap() -> Result<()> {
    let client = Client::new();
    let latest_version = client
        .get(format!("{SETUP}/version"))
        .send()
        .await?
        .text()
        .await?;
    let local_appdata = env::var("LOCALAPPDATA")?;
    let install_dir = PathBuf::from(local_appdata).join("Pekora2");
    fs::create_dir_all(&install_dir)?;
    env::set_current_dir(&install_dir)?;
    if fs::read_to_string("version")
        .map(|x| x == latest_version)
        .unwrap_or(false)
    {
        paris::info!("Latest version {latest_version} installed. Nothing to do.");
        open::that(POST_INSTALL_URL)?;
        return Ok(());
    }
    paris::info!("Updating Pekora");
    let years = client
        .get(format!("{SETUP}/available-years.txt"))
        .send()
        .await?
        .text()
        .await?;
    for year in years.split('\n') {
        paris::info!("Downloading {year} client");
        let client = client
            .get(format!("{SETUP}/{latest_version}-ProjectXApp{year}.zip"))
            .send()
            .await?
            .bytes()
            .await?;
        paris::success!("Download complete");
        paris::info!("Extracting client");
        let client_path = PathBuf::from(format!("Versions/{latest_version}/{year}"));
        fs::create_dir_all(&client_path)?;
        let mut zip = ZipArchive::new(Cursor::new(client))?;
        for i in 0..zip.len() {
            let Ok(mut file) = zip.by_index(i) else {
                paris::warn!("Failed to extract file at index {i}, this client may not work!");
                continue;
            };
            let Some(path) = file.enclosed_name() else {
                paris::warn!(
                    "Filed at index {i} has invalid path and can't be extracted, this client may not work!"
                );
                continue;
            };
            let path = client_path.join(path);
            if file.is_dir() {
                match fs::create_dir_all(&path) {
                    Ok(()) => {
                        paris::success!("[D] {path:?}");
                    }
                    Err(e) => {
                        paris::error!(
                            "Failed to create directory {path:?} ({e}), this client may not work!"
                        );
                    }
                }
            } else {
                if let Some(path) = path.parent()
                    && !path.exists()
                {
                    match fs::create_dir_all(path) {
                        Ok(()) => {
                            paris::success!("[D] {path:?}");
                        }
                        Err(e) => {
                            paris::error!(
                                "Failed to create directory {path:?} ({e}), this client may not work!"
                            );
                        }
                    }
                }
                let Ok(mut fsfile) = File::create(&path) else {
                    paris::error!("Failed to create file {path:?}, this client may not work!");
                    continue;
                };
                match io::copy(&mut file, &mut fsfile) {
                    Ok(_) => {
                        paris::success!("[F] {path:?}");
                    }
                    Err(e) => {
                        paris::error!(
                            "Failed to create file {path:?} ({e}), this client may not work!"
                        );
                    }
                }
            }
        }
        paris::success!("Successfully installed the {year} client");
    }
    paris::success!("All available clients installed");
    let _ = fs::write("version", latest_version);
    paris::info!("Copying self to folder");
    let launcher_path = install_dir.join("PekoraLauncher.exe");
    let _ = fs::copy(current_exe()?, &launcher_path);
    paris::info!("Setting up URI handler");
    utils::register_uri("pekora2-player", &launcher_path)
        .map_err(|e| anyhow!("Error registering URI: {e}"))?;
    paris::success!("Bootstrap finished");
    Ok(())
}
