#[derive(Default, Clone, Debug)]
pub struct Host {
    pub name: &'static str,
    pub version: &'static str,
    pub vendor: &'static str,
    pub knob_preference: Option<KnobPreference>,
    pub language: Option<Language>,
}

/// Create host using crate name, version and authors
#[macro_export]
macro_rules! new_host {
    () => {
        Host::new(
            env!("CARGO_PKG_NAME"),
            env!("CARGO_PKG_VERSION"),
            env!("CARGO_PKG_AUTHORS"),
        )
    };
}

impl Host {
    pub fn new(name: &'static str, version: &'static str, vendor: &'static str) -> Self {
        Self {
            name,
            version,
            vendor,
            ..Default::default()
        }
    }
}

#[derive(Clone, Debug, Copy)]
pub enum KnobPreference {
    Circular,
    Linear,
}

// TODO
#[derive(Clone, Debug, Default, Copy)]
pub enum Language {
    #[default]
    English,
    Spanish,
    French,
    German,
    Italian,
}
