use crate::{parameter::ParameterUpdate, PpqTime, Samples};

use ringbuf::{storage::Storage, traits::*};

/// Events sent to the plugin from the host. Can be passed into the `process` function or queued
/// for the next process call with `queue_event`.
#[derive(Debug, Clone)]
pub struct HostIssuedEvent {
    pub event_type: HostIssuedEventType,
    /// Time in samples from start of next block.
    pub block_time: Samples,
    pub ppq_time: PpqTime,
    pub bus_index: usize,
}

#[derive(Debug, Clone)]
pub enum HostIssuedEventType {
    Midi {
        note_length: Samples,
        midi_data: [u8; 3],
        detune: f32,
    },
    Parameter(ParameterUpdate),
}

/// Events sent to the host from the plugin. Queued in the plugin and the consumed from the `get_events` function.
#[derive(Debug)]
pub enum PluginIssuedEvent {
    /// Plugin changed it's latency. New latency is in samples.
    ChangeLatency(usize),
    /// Plugin changed its editor window size. 0 is width, 1 is height.
    ResizeWindow(usize, usize),
    Parameter(ParameterUpdate),
    UpdateDisplay,
    IOChanged,
}

