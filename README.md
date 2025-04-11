# audio-plugin-host
High-level Rust hosting library for VST2/3 and CLAP. Largely based on [EasyVst](https://github.com/iffyloop/EasyVst) and [vst-rs](https://github.com/RustAudio/vst-rs).

## Setting up VST SDK
1. Download the [VST SDK](https://download.steinberg.net/sdk_downloads/vst-sdk_3.7.7_build-19_2022-12-12.zip) and unzip it.
2. Set the environment variable `VSTSDK_DIR` to the path of the unzipped SDK.

## Usage
For full examples see the `examples` directory.
### Loading
```rust
let host = host::Host::new(
    env!("CARGO_PKG_NAME"),
    env!("CARGO_PKG_VERSION"),
    env!("CARGO_PKG_AUTHORS"),
);

let mut plugin = plugin::load(&PathBuf::from(plugin_path), &host).unwrap();
println!("{:?}\n{:?}", plugin.descriptor, plugin.get_io_configuration());
```

### Processing
```rust
// Audio thread

let process_details = ProcessDetails {
    block_size: 512,
    sample_rate: 44100,
    nanos: start_time
        .duration_since(SystemTime::UNIX_EPOCH)
        .unwrap()
        .as_nanos() as f64,
    ..Default::default()
};

plugin.process(
    &input_buses,
    &mut output_buses,
    events,
    &process_details,
);
```

### Main Loop
```rust
// Main thread

let (width, height) = plugin.show_editor(window_id).unwrap();
window.set_size(width as u32, height as u32).unwrap();

loop {
    let events = plugin.get_events();

    if !events.is_empty() {
        println!("Received events: {:?}", events);
    }

    for event in events {
        match event {
            PluginIssuedEvent::ResizeWindow(width, height) => {
                window.set_size(width as u32, height as u32).unwrap();
            }
            PluginIssuedEvent::Parameter(param) => {
                let param = plugin.get_parameter(param.parameter_id);
                println!("Parameter updated {:?}", param);
            }
            _ => {}
        }
    }
}
```
