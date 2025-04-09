mod clap;
mod vst2;
mod vst3;

use std::path::PathBuf;
use std::sync::{Arc, Mutex};

use ringbuf::HeapProd;

use crate::discovery::*;
use crate::error::{err, Error};
use crate::event::PluginIssuedEvent;
use crate::host::Host;
use crate::plugin::PluginInner;

pub fn load_any(
    path: &PathBuf,
    common: Common,
) -> Result<(Box<dyn PluginInner>, PluginDescriptor), Error> {
    if is_vst2(path, true) {
        return vst2::load(path, common);
    }

    if is_vst3(path) {
        return vst3::load(path);
    }

    if is_clap(path) {
        return clap::load(path);
    }

    return err("The requested path was not a supported plugin format.");
}

/// Common data shared between all plugin formats.
pub struct Common {
    pub host: Host,
    pub plugin_issued_events_producer: HeapProd<PluginIssuedEvent>,
}
