# ID Checker

Small C utilities for generating and installing a board-ID key and checking it
against `/sys/devices/soc0/serial_number`.

On a mismatch, ID Checker creates `/sdcard/Warning` for ROM features to inspect.
It does not reboot, block startup, or show a prompt. The file is removed after a
successful check. Device init/service integration is intentionally left to the
ROM maintainer, so the behavior and recovery path remain visible and controllable.

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

## License

This project is licensed under GNU GPL version 3 only. See [LICENSE](LICENSE).
