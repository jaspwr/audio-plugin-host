use std::path::PathBuf;

use crate::Samples;

#[derive(Clone, Debug)]
pub struct PluginDescriptor {
    pub name: String,
    pub id: String,
    pub path: PathBuf,
    pub version: String,
    pub vendor: String,
    pub format: Format,
    pub initial_latency: Samples,
}

#[derive(Clone, Debug)]
pub enum Format {
    Vst2,
    Vst3,
}

pub fn scan_directory(_path: PathBuf) -> Vec<PluginDescriptor> {
    todo!();
}

pub fn is_vst2(path: &std::path::PathBuf, check_contents: bool) -> bool {
    if !path.exists() {
        return false;
    }

    let path = path.to_string_lossy().to_lowercase();
    if !(path.ends_with(".dll") || path.ends_with(".so") || path.ends_with(".vst")) {
        return false;
    }

    if !check_contents {
        return true;
    }

    let data = std::fs::read(path).expect("Failed to read file");
    match goblin::Object::parse(&data) {
        Ok(goblin::Object::Elf(elf)) => elf.syms.iter().any(|s| {
            elf.strtab
                .get(s.st_name)
                .map(|s| s.map(|s| s == "VSTPluginMain").unwrap_or(false))
                .unwrap_or(false)
        }),
        Ok(goblin::Object::PE(pe)) => pe.exports.iter().any(|e| e.name == Some("VSTPluginMain")),
        _ => false,
    }
}

pub fn is_vst3(path: &std::path::PathBuf) -> bool {
    let path = path.to_string_lossy().to_lowercase();
    path.ends_with(".vst3")
}

pub fn is_clap(path: &std::path::PathBuf) -> bool {
    let path = path.to_string_lossy().to_lowercase();
    path.ends_with(".clap")
}
