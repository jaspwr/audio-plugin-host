use crate::heapless_vec::HeaplessVec;

pub struct AudioBus<'a, T> {
    pub index: usize,
    pub data: &'a mut Vec<Vec<T>>,
}

impl<'a, T> AudioBus<'a, T> {
    pub fn new(index: usize, data: &'a mut Vec<Vec<T>>) -> Self {
        AudioBus { index, data }
    }
}

#[derive(Clone, Debug)]
#[repr(C)]
pub struct IOConfigutaion {
    pub audio_inputs: HeaplessVec<AudioBusDescriptor, 16>,
    pub audio_outputs: HeaplessVec<AudioBusDescriptor, 16>,
}

#[derive(Clone, Debug, Copy)]
#[repr(C)]
pub struct AudioBusDescriptor {
    pub channels: usize,
}
