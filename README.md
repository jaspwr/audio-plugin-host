# audio-plugin-host
Rust hosting library for VST2/3 and CLAP. Largely based on [EasyVst](https://github.com/iffyloop/EasyVst) and [vst-rs](https://github.com/RustAudio/vst-rs).

## Setting up VST SDK
1. Download the [VST SDK](https://download.steinberg.net/sdk_downloads/vst-sdk_3.7.7_build-19_2022-12-12.zip) and unzip it.
2. Set the environment variable `VSTSDK_DIR` to the path of the unzipped SDK.
