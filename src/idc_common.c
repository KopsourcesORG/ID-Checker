#define _POSIX_C_SOURCE 200809L
#include "idc_common.h"

#include <errno.h>
#include <fcntl.h>
#include <openssl/evp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_FILE_SIZE (1024U * 1024U)

int idc_read_file(const char *path, unsigned char **data, size_t *len) {
    FILE *file = fopen(path, "rb");
    long size;
    unsigned char *buffer;
    if (!file) return -1;
    if (fseek(file, 0, SEEK_END) != 0 || (size = ftell(file)) < 0 ||
        (unsigned long)size > MAX_FILE_SIZE || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return -1;
    }
    buffer = malloc((size_t)size + 1);
    if (!buffer) { fclose(file); return -1; }
    *len = fread(buffer, 1, (size_t)size, file);
    if (ferror(file)) { free(buffer); fclose(file); return -1; }
    fclose(file);
    buffer[*len] = 0;
    *data = buffer;
    return 0;
}

int idc_write_file(const char *path, const unsigned char *data, size_t len) {
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    size_t offset = 0;
    if (fd < 0) return -1;
    while (offset < len) {
        ssize_t written = write(fd, data + offset, len - offset);
        if (written < 0 && errno == EINTR) continue;
        if (written <= 0) { close(fd); return -1; }
        offset += (size_t)written;
    }
    if (fsync(fd) != 0 || close(fd) != 0) return -1;
    return 0;
}

int idc_base64_encode(const unsigned char *input, size_t input_len,
                      unsigned char **output, size_t *output_len) {
    size_t capacity;
    int result;
    if (input_len > (size_t)INT_MAX || input_len > (SIZE_MAX - 8) / 2) return -1;
    capacity = 4 * ((input_len + 2) / 3);
    *output = malloc(capacity + 1);
    if (!*output) return -1;
    result = EVP_EncodeBlock(*output, input, (int)input_len);
    if (result < 0) { free(*output); *output = NULL; return -1; }
    *output_len = (size_t)result;
    (*output)[*output_len] = 0;
    return 0;
}

int idc_base64_decode(const unsigned char *input, size_t input_len,
                      unsigned char **output, size_t *output_len) {
    size_t capacity;
    int result;
    if (input_len == 0 || input_len > (size_t)INT_MAX || input_len % 4 != 0) return -1;
    capacity = (input_len / 4) * 3;
    *output = malloc(capacity + 1);
    if (!*output) return -1;
    result = EVP_DecodeBlock(*output, input, (int)input_len);
    if (result < 0) { free(*output); *output = NULL; return -1; }
    while (input_len && input[input_len - 1] == '=') { result--; input_len--; }
    if (result < 0) { free(*output); *output = NULL; return -1; }
    *output_len = (size_t)result;
    (*output)[*output_len] = 0;
    return 0;
}

int idc_sha256_hex(const unsigned char *data, size_t len, char output[65]) {
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len = 0;
    size_t i;
    if (!EVP_Digest(data, len, digest, &digest_len, EVP_sha256(), NULL) || digest_len != 32)
        return -1;
    for (i = 0; i < digest_len; i++) snprintf(output + i * 2, 3, "%02x", digest[i]);
    output[64] = '\0';
    return 0;
}

int idc_extract_digest(const unsigned char *decoded, size_t len, char digest[65]) {
    static const char prefix[] = "IDC1\n";
    size_t i;
    if (len < sizeof(prefix) - 1 + 64 || memcmp(decoded, prefix, sizeof(prefix) - 1) != 0)
        return -1;
    for (i = 0; i < 64; i++) {
        unsigned char c = decoded[sizeof(prefix) - 1 + i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return -1;
        digest[i] = (char)c;
    }
    if (len > sizeof(prefix) - 1 + 64 && decoded[sizeof(prefix) - 1 + 64] != '\n') return -1;
    digest[64] = '\0';
    return 0;
}

int idc_read_board_id(const char *path, char **board_id) {
    unsigned char *data = NULL;
    size_t len = 0;
    if (idc_read_file(path, &data, &len) != 0) return -1;
    while (len && (data[len - 1] == '\n' || data[len - 1] == '\r' || data[len - 1] == ' '))
        data[--len] = 0;
    if (!len || memchr(data, '\0', len) != NULL) { free(data); return -1; }
    *board_id = (char *)data;
    return 0;
}

int idc_install_encoded_key(const char *encoded_path) {
    static const char *directories[] = { "/persist/idc", "/mnt/vendor/persist/idc" };
    unsigned char *encoded = NULL, *decoded = NULL;
    size_t encoded_len = 0, decoded_len = 0;
    size_t i;
    int result = -1;
    if (idc_read_file(encoded_path, &encoded, &encoded_len) != 0 ||
        idc_base64_decode(encoded, encoded_len, &decoded, &decoded_len) != 0) goto done;
    for (i = 0; i < sizeof(directories) / sizeof(directories[0]); i++) {
        struct stat st;
        char destination[256];
        if (stat(directories[i], &st) != 0 || !S_ISDIR(st.st_mode)) continue;
        if (snprintf(destination, sizeof(destination), "%s/id.ike", directories[i]) >= (int)sizeof(destination))
            continue;
        result = idc_write_file(destination, decoded, decoded_len);
        if (result == 0) break;
    }
done:
    free(encoded);
    free(decoded);
    return result;
}
