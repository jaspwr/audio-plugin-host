use cmake;

fn main() {
    println!("cargo:rustc-link-lib=ole32");

    let dst = cmake::build("vst3-wrapper");

    println!("cargo:rustc-link-search=native={}", dst.display());
    // println!("cargo:rustc-link-search=native=./vst3-wrapper/host/build/Release");

    println!("cargo:rustc-link-lib=static=VST_SDK");
    println!("cargo:rustc-link-lib=static=vst3wrapper");
    // println!("cargo:rustc-link-lib=stdc++");
    // println!("cargo:rustc-link-lib=static=base");
    // println!("cargo:rustc-link-lib=static=sdk");
    // println!("cargo:rustc-link-lib=static=sdk_common");
    // println!("cargo:rustc-link-lib=static=sdk_hosting");
    // println!("cargo:rustc-link-lib=static=pluginterfaces");
    // println!("cargo:rustc-link-arg=-D_GLIBCXX_USE_CXX11_ABI=1");
    // println!("cargo:rustc-link-lib=X11"); // Link the X11 library
}
