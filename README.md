# RUST-APT

`rust-apt` provides bindings to `libapt-pkg`.

Currently there is only basic package querying.
The goal is to eventually have all of the functionality that `python-apt` has.

### *This Crate is Under Active Development*

This API is far from what could be considered stable.
If you plan on using it in a real project make sure to pin the exact version.
Breaking changes will be frequent and potentially unnannouned as the API comes together.

Each module has a `raw` submodule containing c++ bindings to `libapt-pkg`.

These are safe to use in terms of memory, but may cause segfaults if you do something wrong.

If you find a way to segfault without using the `libapt-pkg` bindings directly, please report this as a bug.

# Documentation and Examples

For more instructions on how to use `rust-apt` see our [crates.io](https://crates.io/crates/rust-apt) page.

### Getting packages

The `cache.packages()` method is an iterator of all packages that can be sorted with the `PackageSort` struct.

Here is how you could iterate over packages that are upgradable.
They will also be sorted by name using the `.names()` method on `PackageSort`

```rust
use rust_apt::cache::{Cache, PackageSort};

let cache = Cache::new();
let sort = PackageSort::default().upgradable().names();

for pkg in cache.packages(&sort) {
	println!(
		"Package: {} {} is upgradable to {}",
		pkg.name(),
		pkg.installed().unwrap().version(),
		pkg.candidate().unwrap().version(),
	);
}
```

# License Note

This crate is licensed under the GPLv3 or later.

# Building

`libapt-pkg-dev` must be installed.

# Thread safety

It is not advised to use this crate in multiple threads.

You're free to try it but development will not be focused on making this crate thread safe.

# Development

Make sure `cargo` and `rustup` are installed before you run the following commands.

You will need the stable and nightly toolchain. Nightly is only used for `rustfmt`.

Install `just`, a command runner we use to simplify some tasks.

```console
cargo install just
```

Now that `cargo` and `just` are installed, You can setup your dev environment.

`setup-dev` will:

* Install the necessary dependencies with `apt`.

* Ensure the proper toolchains are installed with `rustup`.

* Create `compile_commands.json` with `bear` for better c++ linting

```console
just setup-dev
```

Before you commit, check formatting and basic code QA.

```console
just fmt
just check
```
