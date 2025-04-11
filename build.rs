use std::env;

use cmake::Config;

fn main() {
    let crate_dir = env::var("CARGO_MANIFEST_DIR").unwrap();
    cbindgen::Builder::new()
      .with_crate(crate_dir)
      .generate()
      .expect("Unable to generate bindings")
      .write_to_file("vst3-wrapper/source/bindings.h");

    println!("cargo:rustc-link-lib=ole32");

    let dst = Config::new("vst3-wrapper")
        .build_target("vst3wrapper")
        .profile("Release")
        .no_default_flags(true)
        .build()
        .join("build").join("Release");

    println!("cargo::warning={}", dst.display());

    println!("cargo:rustc-link-search=native={}", dst.display());

    // println!("cargo:rustc-link-search=native=./vst3-wrapper/build/Release");

    println!("cargo:rustc-link-lib=static=vst3wrapper");
    println!("cargo:rustc-link-lib=static=VST_SDK");

    // println!("cargo:rustc-link-lib=stdc++");
    // println!("cargo:rustc-link-lib=static=base");
    // println!("cargo:rustc-link-lib=static=sdk");
    // println!("cargo:rustc-link-lib=static=sdk_common");
    // println!("cargo:rustc-link-lib=static=sdk_hosting");
    // println!("cargo:rustc-link-lib=static=pluginterfaces");
    // println!("cargo:rustc-link-arg=-D_GLIBCXX_USE_CXX11_ABI=1");
    // println!("cargo:rustc-link-lib=X11"); // Link the X11 library
    //

}
