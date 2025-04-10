use cmake::Config;

fn main() {
    println!("cargo:rustc-link-lib=ole32");

    let dst = Config::new("vst3-wrapper")
        .build_target("vst3wrapper")
        .profile("Release")
        .no_default_flags(true)
        .build();

    let profile = std::env::var("PROFILE").unwrap();
    let path = match profile.as_str() {
        "debug" => dst.join("build").join("Debug"),
        "release" => dst.join("build").join("Release"),
        _ => panic!("Unknown profile: {}", profile),
    };

    println!("cargo::warning={}", path.display());

    println!("cargo:rustc-link-search=native={}", path.display());

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
