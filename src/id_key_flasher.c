/* SPDX-License-Identifier: GPL-3.0-only */
#include "idc_common.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#if defined(__aarch64__)
static __thread volatile unsigned char recovery_tls_alignment
    __attribute__((aligned(64)));
#endif

static int is_mounted(const char *path) {
    char line[1024];
    FILE *mounts = fopen("/proc/mounts", "r");
    if (!mounts) return 0;
    while (fgets(line, sizeof(line), mounts)) {
        char mount_path[PATH_MAX];
        if (sscanf(line, "%*s %4095s", mount_path) == 1 && strcmp(mount_path, path) == 0) {
            fclose(mounts);
            return 1;
        }
    }
    fclose(mounts);
    return 0;
}

static int run_mount(const char *path) {
    pid_t child = fork();
    int status;
    if (child < 0) return -1;
    if (child == 0) {
        char *const args[] = { (char *)"mount", (char *)path, NULL };
        execvp(args[0], args);
        _exit(127);
    }
    while (waitpid(child, &status, 0) < 0) {
        if (errno != EINTR) return -1;
    }
    return WIFEXITED(status) && WEXITSTATUS(status) == 0 ? 0 : -1;
}

static const char *ensure_persist_mounted(void) {
    static const char *paths[] = { "/persist", "/mnt/vendor/persist" };
    size_t i;
    for (i = 0; i < sizeof(paths) / sizeof(paths[0]); i++) {
        struct stat st;
        if (stat(paths[i], &st) != 0 || !S_ISDIR(st.st_mode)) continue;
        if (is_mounted(paths[i])) return paths[i];
        if (run_mount(paths[i]) == 0 && is_mounted(paths[i])) return paths[i];
    }
    return NULL;
}

int main(int argc, char **argv) {
    const char *persist_path;
#if defined(__aarch64__)
    recovery_tls_alignment = 0;
#endif
    const char *path = argc > 1 ? argv[1] : "id.ike2";
    if (argc > 2) {
        fprintf(stderr, "Usage: %s [id.ike2]\n", argv[0]);
        return 2;
    }
    persist_path = ensure_persist_mounted();
    if (!persist_path) {
        fprintf(stderr, "ID Key Flasher: could not mount /persist or /mnt/vendor/persist\n");
        return 1;
    }
    if (idc_install_encoded_key(path) != 0) {
        fprintf(stderr, "ID Key Flasher: could not decode key or write under %s\n", persist_path);
        return 1;
    }
    return 0;
}
