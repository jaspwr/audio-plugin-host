use std::ffi::{c_char, c_void};

#[link(name = "vst3wrapper", kind = "static")]
extern "C" {
    pub fn load_plugin(s: *const c_char) -> *const c_void;
    pub fn show_gui(app: *const c_void, window_id: *const c_void) -> Dims;
    pub fn hide_gui(app: *const c_void);
    pub fn name(app: *const c_void) -> *const c_char;
    pub fn vst3_set_sample_rate(app: *const c_void, rate: i32);
    pub fn set_block_size(app: *const c_void, size: i32);
    pub fn process(
        app: *const c_void,
        data: FFIProcessData,
        input: *const *const f32,
        output: *mut *mut f32,
        note_events_count: i32,
        note_events: *const NoteEvent,
        parameter_change_count: *mut i32,
        parameter_changes: *mut ParameterChange,
    );
    fn parameter_names(app: *const c_void) -> *const c_char;
    fn free_parameter_names(names: *const c_char);
    fn set_param_from_ui_thread(app: *const c_void, id: i32, value: f32);

    fn get_data(
        app: *const c_void,
        data_len: *mut i32,
        stream: *mut *const c_void,
    ) -> *const c_void;
    fn free_data_stream(stream: *const c_void);
    fn set_data(app: *const c_void, data: *const c_void, data_len: i32);

    fn consume_parameter(app: *const c_void, param: *mut ParameterEditState) -> bool;
}

#[repr(C)]
#[derive(Debug, Copy, Clone)]
#[allow(non_snake_case)]
pub struct FFIProcessData {
    pub tempo: f64,
    pub timeSigNumerator: ::std::os::raw::c_uint,
    pub timeSigDenominator: ::std::os::raw::c_uint,
    pub currentBeat: f64,
    pub sampleRate: ::std::os::raw::c_int,
    pub blockSize: ::std::os::raw::c_int,
    pub playing: bool,
    pub recording: bool,
    pub cycleActive: bool,
    pub offline: bool,
    pub cycleStartBeats: f64,
    pub cycleEndBeats: f64,
    pub barPosBeats: f64,
    pub systemTime: i64,
}
// #[allow(clippy::unnecessary_operation, clippy::identity_op)]
// const _: () = {
//     ["Size of FFIProcessData"][::std::mem::size_of::<FFIProcessData>() - 32usize];
//     ["Alignment of FFIProcessData"][::std::mem::align_of::<FFIProcessData>() - 8usize];
//     ["Offset of field: FFIProcessData::tempo"]
//         [::std::mem::offset_of!(FFIProcessData, tempo) - 0usize];
//     ["Offset of field: FFIProcessData::timeSigNumerator"]
//         [::std::mem::offset_of!(FFIProcessData, timeSigNumerator) - 8usize];
//     ["Offset of field: FFIProcessData::timeSigDenominator"]
//         [::std::mem::offset_of!(FFIProcessData, timeSigDenominator) - 12usize];
//     ["Offset of field: FFIProcessData::currentBeat"]
//         [::std::mem::offset_of!(FFIProcessData, currentBeat) - 16usize];
//     ["Offset of field: FFIProcessData::sampleRate"]
//         [::std::mem::offset_of!(FFIProcessData, sampleRate) - 24usize];
//     ["Offset of field: FFIProcessData::blockSize"]
//         [::std::mem::offset_of!(FFIProcessData, blockSize) - 28usize];
// };

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
    inital_value: std::os::raw::c_float,
    current_value: std::os::raw::c_float,
    finished: bool,
}
