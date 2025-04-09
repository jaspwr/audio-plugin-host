use std::path::PathBuf;

use crate::error::Error;
use crate::plugin::PluginInner;
use crate::discovery::PluginDescriptor;

pub fn load(path: &PathBuf) -> Result<(Box<dyn PluginInner>, PluginDescriptor), Error> {
    todo!()
}
