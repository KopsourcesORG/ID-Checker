VERSION ?= 0.1.0
GIT_HASH ?= $(shell git rev-parse --short HEAD 2>/dev/null || echo unknown)
CC ?= cc
CFLAGS ?= -O2 -Wall -Wextra -Wpedantic
CPPFLAGS += -DIDC_VERSION=\"$(VERSION)\" -DIDC_GIT_HASH=\"$(GIT_HASH)\"
LDLIBS ?=
NDK_ROOT ?= $(ANDROID_NDK_HOME)
NDK_HOST_TAG ?= linux-x86_64
ANDROID_API ?= 23

COMMON = build/idc_common.o

.PHONY: all clean test android android-binaries android-if-available FORCE
all: build/id-checker build/id-key-generator build/id-key-flasher android-if-available

android-if-available:
	@if test -n "$(NDK_ROOT)"; then $(MAKE) android-binaries; \
	else echo "Android native build skipped (set ANDROID_NDK_HOME to an installed NDK)."; fi
	@if test -n "$(NDK_ROOT)" && command -v gradle >/dev/null 2>&1 && test -d "$${ANDROID_HOME:-/usr/lib/android-sdk}/platforms"; then \
		$(MAKE) android NDK_ROOT="$(NDK_ROOT)"; \
	else \
		echo "Android APK build skipped (NDK, Gradle, and Android SDK platform are required)."; \
	fi

android: android-binaries
	cd android && gradle --no-daemon assembleDebug

android-binaries: | build
	@test -n "$(NDK_ROOT)" || { echo "Set ANDROID_NDK_HOME to the Android NDK root." >&2; exit 2; }
	@set -eu; \
	compiler="$(NDK_ROOT)/toolchains/llvm/prebuilt/$(NDK_HOST_TAG)/bin/aarch64-linux-android$(ANDROID_API)-clang"; \
	test -x "$$compiler" || { echo "Android compiler not found: $$compiler" >&2; exit 2; }; \
	out="build/android/arm64-v8a"; mkdir -p "$$out"; \
	for pair in "id-checker:src/id_checker.c" "id-key-generator:src/id_key_generator.c" "id-key-flasher:src/id_key_flasher.c"; do \
		name=$${pair%%:*}; source=$${pair#*:}; \
		if test "$$name" = id-key-flasher; then link_mode=-static; \
		else link_mode=; fi; \
		"$$compiler" -DIDC_VERSION=\"$(VERSION)\" -DIDC_GIT_HASH=\"$(GIT_HASH)\" \
			$$link_mode -O2 -Wall -Wextra -Wpedantic \
			src/idc_common.c "$$source" -o "$$out/$$name"; \
	done; \
	python3 tools/align_android_tls.py "$$out/id-key-flasher"

build:
	mkdir -p $@

build/idc_common.o: src/idc_common.c src/idc_common.h | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

build/id-checker: src/id_checker.c $(COMMON) FORCE
	$(CC) $(CPPFLAGS) $(CFLAGS) src/id_checker.c $(COMMON) $(LDLIBS) -o $@

build/id-key-generator: src/id_key_generator.c $(COMMON)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDLIBS) -o $@

build/id-key-flasher: src/id_key_flasher.c $(COMMON)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDLIBS) -o $@

test: all
	$(CC) -Isrc $(CPPFLAGS) $(CFLAGS) tests/test_common.c $(COMMON) $(LDLIBS) -o build/test-common
	./build/test-common
	python3 tests/test_align_android_tls.py

clean:
	rm -rf build

FORCE:
