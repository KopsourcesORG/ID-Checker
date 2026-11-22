VERSION ?= 0.1.0
GIT_HASH ?= $(shell git rev-parse --short HEAD 2>/dev/null || echo unknown)
CC ?= cc
CFLAGS ?= -O2 -Wall -Wextra -Wpedantic
CPPFLAGS += -DIDC_VERSION=\"$(VERSION)\" -DIDC_GIT_HASH=\"$(GIT_HASH)\"
LDLIBS += -lcrypto

COMMON = build/idc_common.o

.PHONY: all clean test android android-if-available FORCE
all: build/id-checker build/id-key-generator build/id-key-flasher android-if-available

android-if-available:
	@if command -v gradle >/dev/null 2>&1 && test -d "$${ANDROID_HOME:-/usr/lib/android-sdk}/platforms"; then \
		$(MAKE) android; \
	else \
		echo "Android build skipped (Gradle and Android SDK platform are required)."; \
	fi

android:
	cd android && gradle --no-daemon assembleDebug

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

clean:
	rm -rf build

FORCE:
