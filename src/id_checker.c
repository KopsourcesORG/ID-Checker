/* SPDX-License-Identifier: GPL-3.0-only */
#define _POSIX_C_SOURCE 200809L
#include "idc_common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef IDC_VERSION
#define IDC_VERSION "0.0.0"
#endif
#ifndef IDC_GIT_HASH
#define IDC_GIT_HASH "unknown"
#endif

static const char *key_paths[] = { "/persist/idc/id.ike", "/mnt/vendor/persist/idc/id.ike" };
static const char *board_path = "/sys/devices/soc0/serial_number";
static const char *warning_path = "/sdcard/Warning";

int main(int argc, char **argv) {
    unsigned char *encoded = NULL, *decoded = NULL;
    size_t encoded_len = 0, decoded_len = 0, i;
    char expected[65], actual[65], *board_id = NULL;
    int found = 0, valid = 0;
    if (argc == 2 && strcmp(argv[1], "-v") == 0) {
        printf("ID Checker v%s-g%s \xc2\xa9Kopsources.ORG\n", IDC_VERSION, IDC_GIT_HASH);
        return 0;
    }
    for (i = 0; i < sizeof(key_paths) / sizeof(key_paths[0]); i++) {
        if (idc_read_file(key_paths[i], &encoded, &encoded_len) == 0) { found = 1; break; }
    }
    if (found && idc_base64_decode(encoded, encoded_len, &decoded, &decoded_len) == 0 &&
        idc_extract_digest(decoded, decoded_len, expected) == 0 &&
        idc_read_board_id(board_path, &board_id) == 0 &&
        idc_sha256_hex((const unsigned char *)board_id, strlen(board_id), actual) == 0 &&
        strcmp(expected, actual) == 0) valid = 1;
    if (valid) unlink(warning_path);
    else {
        FILE *marker = fopen(warning_path, "ab");
        if (marker) fclose(marker);
    }
    free(encoded);
    free(decoded);
    free(board_id);
    return 0;
}
