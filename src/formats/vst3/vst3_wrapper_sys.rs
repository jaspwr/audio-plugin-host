use std::{
    ffi::{c_char, c_void},
    path::Path,
};

use ringbuf::{traits::Producer, HeapProd};

use crate::{
    audio_bus::IOConfigutaion, event::PluginIssuedEvent, formats::{Format, PluginDescriptor}, parameter::{Parameter, ParameterUpdate}, ProcessDetails
};

#[link(name = "vst3wrapper", kind = "static")]
extern "C" {
    pub fn load_plugin(
        s: *const c_char,
        plugin_sent_events_producer: *const c_void,
    ) -> *const c_void;
    pub fn show_gui(app: *const c_void, window_id: *const c_void) -> Dims;
    pub fn hide_gui(app: *const c_void);
    pub fn descriptor(app: *const c_void) -> FFIPluginDescriptor;
    pub fn io_config(app: *const c_void) -> IOConfigutaion;
    pub fn process(
        app: *const c_void,
        data: ProcessDetails,
        input: *mut *mut f32,
        output: *mut *mut f32,
        note_events_count: i32,
        note_events: *const NoteEvent,
        parameter_change_count: *mut i32,
        parameter_changes: *mut ParameterUpdate,
    );
    pub fn set_param_in_edit_controller(app: *const c_void, id: i32, value: f32);
    pub fn get_parameter(app: *const c_void, id: i32) -> ParameterFFI;

    pub fn get_data(
        app: *const c_void,
        data_len: *mut i32,
        stream: *mut *const c_void,
    ) -> *const c_void;
    pub fn free_data_stream(stream: *const c_void);
    pub fn set_data(app: *const c_void, data: *const c_void, data_len: i32);

    fn free_string(str: *const c_char);
}

#[no_mangle]
pub extern "C" fn send_event_to_host(
    event: *const PluginIssuedEvent,
    plugin_sent_events_producer: *const c_void,
) {
    let event = unsafe { &*event };
    let producer = unsafe { &mut *(plugin_sent_events_producer as *mut HeapProd<PluginIssuedEvent>) };
    let _ = producer.try_push(event.clone());
}

#[repr(C)]
#[allow(non_snake_case)]
#[derive(Debug, Copy, Clone)]
pub struct FFIPluginDescriptor {
    name: *const std::os::raw::c_char,
    vendor: *const std::os::raw::c_char,
    version: *const std::os::raw::c_char,
    id: *const std::os::raw::c_char,
    initial_latency: std::os::raw::c_int,
}

impl FFIPluginDescriptor {
    pub fn to_plugin_descriptor(self, plugin_path: &Path) -> PluginDescriptor {
        PluginDescriptor {
            name: load_and_free_c_string(self.name),
            vendor: load_and_free_c_string(self.vendor),
            version: load_and_free_c_string(self.version),
            id: load_and_free_c_string(self.id),
            initial_latency: self.initial_latency as usize,
            path: plugin_path.to_path_buf(),
            format: Format::Vst3,
        }
    }
}

#[repr(C)]
#[allow(non_snake_case)]
#[derive(Debug, Copy, Clone)]
pub struct Dims {
    pub width: std::os::raw::c_int,
    pub height: std::os::raw::c_int,
}

#[repr(C)]
#[allow(non_snake_case)]
#[derive(Debug, Copy, Clone)]
pub struct NoteEvent {
    pub on: bool,
    pub note: std::os::raw::c_int,
    pub velocity: std::os::raw::c_float,
    pub tuning: std::os::raw::c_float,
    pub channel: std::os::raw::c_int,
    pub samples_offset: std::os::raw::c_int,
    pub time_beats: std::os::raw::c_float,
}

#[repr(C)]
#[allow(non_snake_case)]
#[derive(Debug, Copy, Clone)]
pub struct ParameterChange {
    id: std::os::raw::c_int,
    value: std::os::raw::c_float,
}

#[repr(C)]
#[allow(non_snake_case)]
#[derive(Debug, Copy, Clone)]
struct ParameterEditState {
    id: std::os::raw::c_int,
    initial_value: std::os::raw::c_float,
    current_value: std::os::raw::c_float,
    finished: bool,
}


#[repr(C)]
#[allow(non_snake_case)]
#[derive(Debug, Copy, Clone)]
pub struct ParameterFFI {
    id: std::os::raw::c_int,
    name: *const std::os::raw::c_char,
    index: std::os::raw::c_int,
    value: std::os::raw::c_float,
    formatted_value: *const std::os::raw::c_char,
}

impl ParameterFFI {
    pub fn to_parameter(self) -> Parameter {
        crate::parameter::Parameter {
            id: self.id,
            name: load_and_free_c_string(self.name),
            index: self.index,
            value: self.value,
            formatted_value: load_and_free_c_string(self.formatted_value),
        }
    }
}

fn load_and_free_c_string(s: *const c_char) -> String {
    if s.is_null() {
        return "?".to_string();
    }

    let c_str = unsafe { std::ffi::CStr::from_ptr(s) };
    let str = c_str.to_string_lossy().into_owned();
    unsafe { free_string(s) };
    str
}
