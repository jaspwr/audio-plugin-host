use std::ffi::c_void;
use std::path::Path;

use crate::audio_bus::IOConfigutaion;
use crate::discovery::PluginDescriptor;
use crate::error::Error;
use crate::plugin::PluginInner;

use super::Format;

mod vst3_wrapper_sys;

struct Vst3 {
    app: *const c_void,
}

pub fn load(path: &Path) -> Result<(Box<dyn PluginInner>, PluginDescriptor), Error> {
    let app = unsafe {
        let plugin_path = std::ffi::CString::new(path.to_str().unwrap()).unwrap();
        vst3_wrapper_sys::load_plugin(plugin_path.as_ptr())
    };

    let processor = Vst3 {
        app,
    };
    let name = processor.get_name();
    let vendor = "idk".to_string();

    let descriptor = PluginDescriptor {
        name: name.to_string(),
        vendor: vendor.to_string(),
        id: "1".to_string(),
        path: path.to_path_buf(),
        version: "1".to_string(),
        format: Format::Vst3,
        initial_latency: 0,
    };

    Ok(
        (
            Box::new(processor),
            descriptor,
        )
    )
}

impl Vst3 {
    fn get_name(&self) -> String {
        unsafe {
            let name = vst3_wrapper_sys::name(self.app);
            std::ffi::CStr::from_ptr(name)
                .to_str()
                .unwrap_or("[invaid plugin name]")
                .to_string()
        }
    }
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
        todo!()
    }

    fn show_editor(&mut self, window_id: *mut std::ffi::c_void) -> Result<(usize, usize), Error> {
        let dims = unsafe {
            // show_gui(self.app, window_id as *const c_void);
            vst3_wrapper_sys::show_gui(self.app, window_id as *const c_void)
        };

        return Ok((dims.width as usize, dims.height as usize));
    }

    fn hide_editor(&mut self) {
        todo!()
    }

    fn change_sample_rate(&mut self, rate: crate::SampleRate) {
        todo!()
    }

    fn change_block_size(&mut self, size: crate::BlockSize) {
        todo!()
    }

    fn suspend(&mut self) {
        todo!()
    }

    fn resume(&mut self) {
        todo!()
    }

    fn get_io_configuration(&self) -> crate::audio_bus::IOConfigutaion {
        IOConfigutaion {
            audio_inputs: vec![],
            audio_outputs: vec![],
        }
    }

    fn get_latency(&mut self) -> crate::Samples {
        todo!()
    }
}
