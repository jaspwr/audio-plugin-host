#[derive(Debug)]
pub struct Parameter {
    pub id: i32,
    pub name: String,
    pub index: i32,
    pub value: f32,
    pub formatted_value: String,
}

#[derive(Debug, Clone)]
pub struct ParameterUpdate {
    pub parameter_id: i32,
    pub current_value: f32,
    /// Value at start of edit. For example, the value before the user started dragging a knob
    /// in the plugin editor. Not required to be set when sending events to the plugin; just
    /// used for implementing undo/redo in the host.
    pub inital_value: Option<f32>,
    /// If `Some`, this is a parameter update triggered by a user action in the plugin editor. If
    /// `true`, the user has just released the control and this is the final value.
    pub end_edit: Option<bool>,
    pub formatted_value: Option<String>,
}

impl ParameterUpdate {
    pub fn new(id: i32, value: f32) -> Self {
        ParameterUpdate {
            parameter_id: id,
            current_value: value,
            inital_value: None,
            end_edit: None,
            formatted_value: None,
        }
    }
}
