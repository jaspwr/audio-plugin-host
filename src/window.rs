#[cfg(target_os = "macos")]
use baseview::{copy_to_clipboard, MouseEvent};
use baseview::{
    Event, EventStatus, PhySize, Window, WindowEvent, WindowHandler, WindowScalePolicy,
};

struct OpenWindowExample {}

impl WindowHandler for OpenWindowExample {
    fn on_frame(&mut self, _window: &mut Window) {}

    fn on_event(&mut self, _window: &mut Window, event: Event) -> EventStatus {
        EventStatus::Captured
    }
}

pub struct PluginWindow {
    pub handle: *mut std::ffi::c_void,
}

impl PluginWindow {
    pub fn new() -> Self {
        let window_open_options = baseview::WindowOpenOptions {
            title: "baseview".into(),
            size: baseview::Size::new(512.0, 512.0),
            scale: WindowScalePolicy::SystemScaleFactor,
        };

        let handle = Window::open_blocking(window_open_options, |window| {
            // let ctx = unsafe { softbuffer::Context::new(window) }.unwrap();
            // let mut surface = unsafe { softbuffer::Surface::new(&ctx, window) }.unwrap();
            // surface
            //     .resize(NonZeroU32::new(512).unwrap(), NonZeroU32::new(512).unwrap())
            //     .unwrap();

            OpenWindowExample {}
        });

        PluginWindow {
            handle,
        }
    }
}

impl Drop for PluginWindow {
    fn drop(&mut self) {
        // todo!()
    }
}
