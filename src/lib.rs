pub mod plugin;
pub mod event;
pub mod host;
pub mod audio_bus;
pub mod parameter;
pub mod discovery;
pub mod error;

mod formats;

pub type SampleRate = usize;
pub type BlockSize = usize;
pub type Tempo = f64;
pub type PpqTime = f64;
pub type Samples = usize;

///////////////////// Unsorted

#[repr(C)]
#[derive(Clone)]
pub struct ProcessDetails {
    pub sample_rate: SampleRate,
    pub block_size: BlockSize,
    pub tempo: Tempo,
    pub player_time: PpqTime,
    pub time_signature_numerator: usize,
    pub time_signature_denominator: usize,
    pub cycle: Option<(PpqTime, PpqTime)>,
    pub playing_state: PlayingState,
    pub bar_start_pos: PpqTime,
    pub nanos: f64,
}

impl Default for ProcessDetails {
    fn default() -> Self {
        ProcessDetails {
            sample_rate: 44100,
            block_size: 512,
            tempo: 120.0,
            player_time: 0.0,
            time_signature_numerator: 4,
            time_signature_denominator: 4,
            cycle: None,
            playing_state: PlayingState::Stopped,
            bar_start_pos: 0.0,
            nanos: 0.0,
        }
    }
}

#[derive(Default, PartialEq, Eq, Clone, Copy, Debug)]
pub enum PlayingState {
    #[default]
    Stopped,
    Playing,
    Recording,
    OfflineRendering,
}

impl PlayingState {
    pub fn is_playing(&self) -> bool {
        match self {
            PlayingState::Stopped => false,
            PlayingState::Playing => true,
            PlayingState::Recording => true,
            PlayingState::OfflineRendering => true,
        }
    }
}

///////////////////////

pub fn add(left: u64, right: u64) -> u64 {
    left + right
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn it_works() {
        let result = add(2, 2);
        assert_eq!(result, 4);
    }
}
