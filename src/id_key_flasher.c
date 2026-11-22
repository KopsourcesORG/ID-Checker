/* SPDX-License-Identifier: GPL-3.0-only */
#include "idc_common.h"

#include <stdio.h>

int main(int argc, char **argv) {
    const char *path = argc > 1 ? argv[1] : "id.ike2";
    if (argc > 2) {
        fprintf(stderr, "Usage: %s [id.ike2]\n", argv[0]);
        return 2;
    }
    if (idc_install_encoded_key(path) != 0) {
        fprintf(stderr, "ID Key Flasher: could not decode key or find a mounted persist directory\n");
        return 1;
    }
    return 0;
}
