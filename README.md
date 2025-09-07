# Pekora bootstrapper
Bootstrapper for [pekora.org](https://pekora.org/auth/home). Rewritten to be faster and less buggy, along with providing more debugging information for any bugs that may arise.
## Install
Download the latest version of the bootstrapper from [releases](https://github.com/kerosina/Pekora-Bootstrapper/releases). These binaries are compiled directly from source through Github Actions, and so are verifiably built from the code on this repository.
## Compiling
If you'd like, you can compile the bootstrapper yourself.

1. Get the Rust toolchain (https://rustup.rs/).
2. On the root of the source, execute `cargo build --release`. The binary will now be located in `target/release/pekora-bootstrapper.exe`. You may also run `cargo run --release` to automatically install Pekora from this source.