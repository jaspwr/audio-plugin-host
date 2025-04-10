use std::path::Path;

use crate::discovery::PluginDescriptor;
use crate::error::Error;
use crate::plugin::PluginInner;

pub fn load(_path: &Path) -> Result<(Box<dyn PluginInner>, PluginDescriptor), Error> {
    todo!()
}
