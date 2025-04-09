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
pub struct IOConfigutaion {
    pub audio_inputs: Vec<AudioBusDescriptor>,
    pub audio_outputs: Vec<AudioBusDescriptor>,
}

#[derive(Clone, Debug)]
pub struct AudioBusDescriptor {
    pub channels: usize,
}

