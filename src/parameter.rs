#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};

#[derive(Debug)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct Parameter {
    pub id: i32,
    pub name: String,
    pub index: i32,
    pub value: f32,
    pub formatted_value: String,
    pub hidden: bool,
    pub can_automate: bool,
    pub is_wrap_around: bool,
    pub read_only: bool,
}

#[derive(Debug, Clone)]
#[repr(C)]
pub struct ParameterUpdate {
    pub parameter_id: i32,
    pub current_value: f32,
    /// Value at start of edit. For example, the value before the user started dragging a knob
    /// in the plugin editor. Not required to be set when sending events to the plugin; just
    /// used for implementing undo/redo in the host.
    pub initial_value: f32,
    ///  If `true`, the user has just released the control and this is the final value.
    pub end_edit: bool,
}

impl ParameterUpdate {
    pub fn new(id: i32, value: f32) -> Self {
        ParameterUpdate {
            parameter_id: id,
            current_value: value,
            initial_value: f32::NAN,
            end_edit: false,
        }
    }
}
