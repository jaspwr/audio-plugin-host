use crate::heapless_vec::HeaplessVec;

pub struct AudioBus<'a, T> {
    pub data: &'a mut Vec<Vec<T>>,
    owned_data: *mut Vec<Vec<T>>,
}

impl<'a, T> AudioBus<'a, T> {
    pub fn new(data: &'a mut Vec<Vec<T>>) -> Self {
        AudioBus {
            data,
            owned_data: std::ptr::null_mut(),
        }
    }

    pub fn channels(&self) -> usize {
        self.data.len()
    }
}

#[derive(Clone, Debug)]
#[repr(C)]
/// Input and output configuration for the plugin.
pub struct IOConfigutaion {
    pub audio_inputs: HeaplessVec<AudioBusDescriptor, 16>,
    pub audio_outputs: HeaplessVec<AudioBusDescriptor, 16>,
    pub event_inputs_count: i32,
}

impl IOConfigutaion {
    pub fn matches<'a, T>(&self, inputs: &[AudioBus<'a, T>], outputs: &[AudioBus<'a, T>]) -> bool {
        if self.audio_inputs.len() != inputs.len() || self.audio_outputs.len() != outputs.len() {
            return false;
        }

        #[allow(clippy::needless_range_loop)]
        for i in 0..self.audio_inputs.len() {
            if self.audio_inputs[i].channels != inputs[i].channels() {
                return false;
            }
        }

        #[allow(clippy::needless_range_loop)]
        for i in 0..self.audio_outputs.len() {
            if self.audio_outputs[i].channels != outputs[i].channels() {
                return false;
            }
        }

        true
    }
}

#[derive(Clone, Debug, Copy)]
#[repr(C)]
pub struct AudioBusDescriptor {
    pub channels: usize,
}

impl<T> AudioBus<'_, T>
where
    T: Default + Clone,
{
    pub fn new_alloced(block_size: usize, channels: usize) -> Self {
        let buffer = vec![vec![T::default(); block_size]; channels];
        let ptr = Box::into_raw(Box::new(buffer));
        AudioBus {
            data: unsafe { &mut *ptr },
            owned_data: ptr,
        }
    }
}

impl<T> Drop for AudioBus<'_, T> {
    fn drop(&mut self) {
        if !self.owned_data.is_null() {
            unsafe {
                drop(Box::from_raw(self.owned_data));
            }
        }
    }
}
