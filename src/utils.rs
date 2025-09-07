use std::path::Path;
use windows_registry::{CURRENT_USER, Value};

pub fn register_uri(uri_scheme: &str, exe_path: &Path) -> Result<(), Box<dyn std::error::Error>> {
    let classes_root = CURRENT_USER.create(r"Software\Classes")?;
    let scheme_key = classes_root.create(uri_scheme)?;
    scheme_key.set_value("", &Value::from(format!("URL:{uri_scheme}").as_str()))?;
    scheme_key.set_value("URL Protocol", &Value::from(""))?;
    let shell_key = scheme_key.create("shell")?;
    let open_key = shell_key.create("open")?;
    let command_key = open_key.create("command")?;

    let command = format!("\"{}\" \"%1\"", exe_path.display());
    command_key.set_value("", &Value::from(command.as_str()))?;

    Ok(())
}
