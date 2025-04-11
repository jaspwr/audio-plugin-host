use std::collections::HashMap;
use std::ffi::c_void;
use std::path::Path;

use ringbuf::traits::{Consumer, Producer};
use ringbuf::{HeapProd, HeapRb, SharedRb};
use vst3_wrapper_sys::{descriptor, get_parameter, set_param_in_edit_controller};

use crate::discovery::PluginDescriptor;
use crate::error::Error;
use crate::event::HostIssuedEventType;
use crate::heapless_vec::HeaplessVec;
use crate::parameter::ParameterUpdate;
use crate::plugin::PluginInner;
use crate::Samples;
use crate::{
    audio_bus::IOConfigutaion,
    event::{HostIssuedEvent, PluginIssuedEvent},
};

use super::{Common, Format};

mod vst3_wrapper_sys;

struct Vst3 {
    app: *const c_void,
    plugin_issued_events_producer: Box<HeapProd<PluginIssuedEvent>>,
    param_updates_for_edit_controller: HeapRb<ParameterUpdate>,
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
        param_updates_for_edit_controller: HeapRb::new(512),
    };

    Ok((Box::new(processor), descriptor))
}

impl PluginInner for Vst3 {
    fn process(
        &mut self,
        inputs: &Vec<crate::audio_bus::AudioBus<f32>>,
        outputs: &mut Vec<crate::audio_bus::AudioBus<f32>>,
        events: Vec<HostIssuedEvent>,
        process_details: &crate::ProcessDetails,
    ) {
        for update in last_param_updates(&events) {
            self.param_updates_for_edit_controller.try_push(update);
        }
    }

    fn set_preset_data(&mut self, data: Vec<u8>) -> Result<(), String> {
        unsafe {
            vst3_wrapper_sys::set_data(self.app, data.as_ptr() as *const c_void, data.len() as i32);
            Ok(())
        }
    }

    fn get_preset_data(&mut self) -> Result<Vec<u8>, String> {
        unsafe {
            let mut len = 0;
            let mut stream = std::ptr::null();

            let data = vst3_wrapper_sys::get_data(
                self.app,
                &mut len as *mut i32,
                &mut stream as *mut *const c_void,
            );

            if data.is_null() {
                return Err("Failed to get preset data".to_string());
            }

            let data = std::slice::from_raw_parts(data as *const u8, len as usize)
                .to_vec()
                .clone();

            vst3_wrapper_sys::free_data_stream(stream);

            Ok(data)
        }
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
        unsafe { vst3_wrapper_sys::io_config(self.app) }
    }

    fn get_latency(&mut self) -> crate::Samples {
        0
    }

    fn editor_updates(&mut self) {
        while let Some(update) = self.param_updates_for_edit_controller.try_pop() {
            if !update.current_value.is_nan() {
                unsafe {
                    set_param_in_edit_controller(
                        self.app,
                        update.parameter_id,
                        update.current_value,
                    )
                };
            }
        }
    }
}

/// Gets param updates taking the final update at the latest sample for each parameter
fn last_param_updates(events: &[HostIssuedEvent]) -> Vec<ParameterUpdate> {
    struct ParamUpdate<'a> {
        param: &'a ParameterUpdate,
        block_time: Samples,
    }

    // FIXME: Make real-time safe
    let mut updates: HashMap<i32, ParamUpdate> = HashMap::new();
    for event in events {
        if let HostIssuedEventType::Parameter(ref param) = event.event_type {
            updates.insert(
                param.parameter_id,
                ParamUpdate {
                    param,
                    block_time: event.block_time,
                },
            );
        }
    }

    updates.iter().map(|(_, v)| v.param.clone()).collect()
}
