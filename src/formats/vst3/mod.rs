use std::ffi::c_void;
use std::path::Path;

use ringbuf::HeapProd;
use vst3_wrapper_sys::{descriptor, get_parameter};

use crate::discovery::PluginDescriptor;
use crate::error::Error;
use crate::plugin::PluginInner;
use crate::{audio_bus::IOConfigutaion, event::PluginIssuedEvent};

use super::{Common, Format};

mod vst3_wrapper_sys;

struct Vst3 {
    app: *const c_void,
    plugin_issued_events_producer: Box<HeapProd<PluginIssuedEvent>>,
}

pub fn load(
    path: &Path,
    common: Common,
) -> Result<(Box<dyn PluginInner>, PluginDescriptor), Error> {
    let plugin_issued_events_producer = Box::new(common.plugin_issued_events_producer);

    let app = unsafe {
        let plugin_path = std::ffi::CString::new(path.to_str().unwrap()).unwrap();
        vst3_wrapper_sys::load_plugin(
            plugin_path.as_ptr(),
            &*plugin_issued_events_producer as *const _ as *const c_void,
        )
    };

    let descriptor = unsafe { descriptor(app) }.to_plugin_descriptor(path);
    let processor = Vst3 {
        app,
        plugin_issued_events_producer,
    };

    Ok((Box::new(processor), descriptor))
}

impl PluginInner for Vst3 {
    fn process(
        &mut self,
        inputs: &Vec<crate::audio_bus::AudioBus<f32>>,
        outputs: &mut Vec<crate::audio_bus::AudioBus<f32>>,
        events: Vec<crate::event::HostIssuedEvent>,
        process_details: &crate::ProcessDetails,
    ) {
    }

    fn set_preset_data(&mut self, data: Vec<u8>) -> Result<(), String> {
        todo!()
    }

    fn get_preset_data(&mut self) -> Result<Vec<u8>, String> {
        todo!()
    }

    fn get_preset_name(&mut self, id: i32) -> Result<String, String> {
        todo!()
    }

    fn set_preset(&mut self, id: i32) -> Result<(), String> {
        todo!()
    }

    fn get_parameter(&self, id: i32) -> crate::parameter::Parameter {
        unsafe { get_parameter(self.app, id) }.to_parameter()
    }

    fn show_editor(&mut self, window_id: *mut std::ffi::c_void) -> Result<(usize, usize), Error> {
        let dims = unsafe { vst3_wrapper_sys::show_gui(self.app, window_id as *const c_void) };

        return Ok((dims.width as usize, dims.height as usize));
    }

    fn hide_editor(&mut self) {
        unsafe { vst3_wrapper_sys::hide_gui(self.app) };
    }

    fn suspend(&mut self) {}

    fn resume(&mut self) {}

    fn get_io_configuration(&self) -> crate::audio_bus::IOConfigutaion {
        IOConfigutaion {
            audio_inputs: vec![],
            audio_outputs: vec![],
        }
    }

    fn get_latency(&mut self) -> crate::Samples {
        0
    }
}
