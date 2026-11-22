/* SPDX-License-Identifier: GPL-3.0-only */
#define _POSIX_C_SOURCE 200809L
#include "idc_common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(const char *name) {
    printf("Usage: %s -id <board-id>\nWrites id.ike and id.ike2 in the current directory.\n", name);
}

int main(int argc, char **argv) {
    char digest[65];
    unsigned char *plain = NULL, *ike = NULL, *ike2 = NULL;
    size_t plain_len, ike_len = 0, ike2_len = 0;
    int result = 1;
    if (argc == 1 || (argc == 2 && strcmp(argv[1], "-h") == 0)) {
        usage(argv[0]);
        return 0;
    }
    if (argc != 3 || strcmp(argv[1], "-id") != 0 || !argv[2][0]) {
        usage(argv[0]);
        return 2;
    }
    if (idc_sha256_hex((const unsigned char *)argv[2], strlen(argv[2]), digest) != 0) goto done;
    plain_len = strlen("IDC1\n") + 64 + 1;
    plain = malloc(plain_len + 1);
    if (!plain) goto done;
    snprintf((char *)plain, plain_len + 1, "IDC1\n%s\n", digest);
    if (idc_base64_encode(plain, plain_len, &ike, &ike_len) != 0 ||
        idc_base64_encode(ike, ike_len, &ike2, &ike2_len) != 0) goto done;
    if (idc_write_file("id.ike", ike, ike_len) != 0 ||
        idc_write_file("id.ike2", ike2, ike2_len) != 0) goto done;
    result = 0;
done:
    if (result) fprintf(stderr, "ID Key Generator: failed to create key files\n");
    free(plain);
    free(ike);
    free(ike2);
    return result;
}
