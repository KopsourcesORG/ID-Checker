# ID Checker

Small C utilities for generating and installing a board-ID key and checking it
against `/sys/devices/soc0/serial_number`.
On a mismatch, ID Checker will reboot device into fastboot.

The file format is a transport encoding, not encryption or tamper-proofing.
Do not treat a local marker file or SHA-256 comparison as a secure DRM boundary:
users with system access can alter either. This project does not obfuscate its
binaries.

## Build

Host builds require a C compiler and Make. The C binaries have no runtime
dependency beyond the platform C library.

```sh
make VERSION=0.1.0
make test
```

Targets are `build/id-checker`, `build/id-key-generator`, and
`build/id-key-flasher`. To cross-compile Android shell executables, install the
Android NDK and run:

```sh
ANDROID_NDK_HOME=/path/to/ndk make android-binaries
```

The native target is `arm64-v8a` with API level 23. Output binaries are placed
under `build/android/arm64-v8a/`. `make` builds these automatically when
`ANDROID_NDK_HOME` is set. The standalone Android app project is in `android/`
and writes the generated keys to the device's Downloads collection; its APK is
built automatically when Gradle and an Android SDK platform are installed.
The app creates a recovery-flashable ZIP containing the key, the installer
scripts from `flashable/`, and the arm64 `id-key-flasher` compiled by
`make android-binaries` before the APK build.

`id-key-flasher` is linked statically for recovery compatibility, so TWRP does
not need to provide Android's `/system/bin/linker64` or userspace libraries.
The NDK linker currently emits an undersized AArch64 TLS alignment, so the
build validates and corrects the `PT_TLS` alignment after linking. The flasher
checks `/proc/mounts`, mounts an available persist path when needed, then writes
the key. Use the binary from `build/android/arm64-v8a/`, not the host-only
executable in `build/`.

## License

This project is licensed under GNU GPL version 3 only. See [LICENSE](LICENSE).
