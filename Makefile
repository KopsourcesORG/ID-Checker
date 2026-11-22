VERSION ?= 0.1.0
CC ?= cc
CFLAGS ?= -O2 -Wall -Wextra -Wpedantic
CPPFLAGS += -DIDC_VERSION=\"$(VERSION)\"
LDLIBS += -lcrypto

COMMON = build/idc_common.o

.PHONY: all clean test
all: build/id-checker build/id-key-generator build/id-key-flasher

build:
	mkdir -p $@

build/idc_common.o: src/idc_common.c src/idc_common.h | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

build/id-checker: src/id_checker.c $(COMMON)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDLIBS) -o $@

build/id-key-generator: src/id_key_generator.c $(COMMON)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDLIBS) -o $@

build/id-key-flasher: src/id_key_flasher.c $(COMMON)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDLIBS) -o $@

test: all
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_common.c $(COMMON) $(LDLIBS) -o build/test-common
	./build/test-common

clean:
	rm -rf build
