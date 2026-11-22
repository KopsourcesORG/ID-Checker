#define _POSIX_C_SOURCE 200809L
#include "idc_common.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_FILE_SIZE (1024U * 1024U)

static const char b64_alphabet[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static uint32_t rotate_right(uint32_t value, unsigned int count) {
    return (value >> count) | (value << (32U - count));
}

static void sha256_block(const unsigned char block[64], uint32_t state[8]) {
    static const uint32_t k[64] = {
        0x428a2f98U,0x71374491U,0xb5c0fbcfU,0xe9b5dba5U,0x3956c25bU,0x59f111f1U,0x923f82a4U,0xab1c5ed5U,
        0xd807aa98U,0x12835b01U,0x243185beU,0x550c7dc3U,0x72be5d74U,0x80deb1feU,0x9bdc06a7U,0xc19bf174U,
        0xe49b69c1U,0xefbe4786U,0x0fc19dc6U,0x240ca1ccU,0x2de92c6fU,0x4a7484aaU,0x5cb0a9dcU,0x76f988daU,
        0x983e5152U,0xa831c66dU,0xb00327c8U,0xbf597fc7U,0xc6e00bf3U,0xd5a79147U,0x06ca6351U,0x14292967U,
        0x27b70a85U,0x2e1b2138U,0x4d2c6dfcU,0x53380d13U,0x650a7354U,0x766a0abbU,0x81c2c92eU,0x92722c85U,
        0xa2bfe8a1U,0xa81a664bU,0xc24b8b70U,0xc76c51a3U,0xd192e819U,0xd6990624U,0xf40e3585U,0x106aa070U,
        0x19a4c116U,0x1e376c08U,0x2748774cU,0x34b0bcb5U,0x391c0cb3U,0x4ed8aa4aU,0x5b9cca4fU,0x682e6ff3U,
        0x748f82eeU,0x78a5636fU,0x84c87814U,0x8cc70208U,0x90befffaU,0xa4506cebU,0xbef9a3f7U,0xc67178f2U
    };
    uint32_t words[64], a, b, c, d, e, f, g, h;
    size_t i;
    for (i = 0; i < 16; i++) {
        size_t j = i * 4;
        words[i] = ((uint32_t)block[j] << 24) | ((uint32_t)block[j + 1] << 16) |
                   ((uint32_t)block[j + 2] << 8) | (uint32_t)block[j + 3];
    }
    for (; i < 64; i++) {
        uint32_t s0 = rotate_right(words[i - 15], 7) ^ rotate_right(words[i - 15], 18) ^ (words[i - 15] >> 3);
        uint32_t s1 = rotate_right(words[i - 2], 17) ^ rotate_right(words[i - 2], 19) ^ (words[i - 2] >> 10);
        words[i] = words[i - 16] + s0 + words[i - 7] + s1;
    }
    a = state[0]; b = state[1]; c = state[2]; d = state[3];
    e = state[4]; f = state[5]; g = state[6]; h = state[7];
    for (i = 0; i < 64; i++) {
        uint32_t s1 = rotate_right(e, 6) ^ rotate_right(e, 11) ^ rotate_right(e, 25);
        uint32_t choice = (e & f) ^ (~e & g);
        uint32_t t1 = h + s1 + choice + k[i] + words[i];
        uint32_t s0 = rotate_right(a, 2) ^ rotate_right(a, 13) ^ rotate_right(a, 22);
        uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
        uint32_t t2 = s0 + majority;
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }
    state[0] += a; state[1] += b; state[2] += c; state[3] += d;
    state[4] += e; state[5] += f; state[6] += g; state[7] += h;
}

int idc_read_file(const char *path, unsigned char **data, size_t *len) {
    FILE *file = fopen(path, "rb");
    size_t capacity = 4096, used = 0;
    unsigned char *buffer;
    if (!file) return -1;
    buffer = malloc(capacity + 1);
    if (!buffer) { fclose(file); return -1; }
    for (;;) {
        size_t count;
        if (used == capacity) {
            size_t next;
            unsigned char *grown;
            if (capacity == MAX_FILE_SIZE) {
                int extra = fgetc(file);
                if (extra != EOF) { free(buffer); fclose(file); return -1; }
                if (ferror(file)) { free(buffer); fclose(file); return -1; }
                break;
            }
            next = capacity > MAX_FILE_SIZE / 2 ? MAX_FILE_SIZE : capacity * 2;
            grown = realloc(buffer, next + 1);
            if (!grown) { free(buffer); fclose(file); return -1; }
            buffer = grown;
            capacity = next;
        }
        count = fread(buffer + used, 1, capacity - used, file);
        used += count;
        if (count == 0) {
            if (ferror(file)) { free(buffer); fclose(file); return -1; }
            break;
        }
    }
    fclose(file);
    buffer[used] = 0;
    *len = used;
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
    size_t capacity, i, o = 0;
    if (input_len > SIZE_MAX - 2 || (input_len + 2) / 3 > (SIZE_MAX - 1) / 4) return -1;
    capacity = 4 * ((input_len + 2) / 3);
    *output = malloc(capacity + 1);
    if (!*output) return -1;
    for (i = 0; i < input_len; i += 3) {
        size_t remain = input_len - i;
        unsigned int value = (unsigned int)input[i] << 16;
        if (remain > 1) value |= (unsigned int)input[i + 1] << 8;
        if (remain > 2) value |= input[i + 2];
        (*output)[o++] = b64_alphabet[(value >> 18) & 63];
        (*output)[o++] = b64_alphabet[(value >> 12) & 63];
        (*output)[o++] = remain > 1 ? b64_alphabet[(value >> 6) & 63] : '=';
        (*output)[o++] = remain > 2 ? b64_alphabet[value & 63] : '=';
    }
    *output_len = o;
    (*output)[*output_len] = 0;
    return 0;
}

static int base64_value(unsigned char c) {
    const char *found = strchr(b64_alphabet, (int)c);
    return found ? (int)(found - b64_alphabet) : -1;
}

int idc_base64_decode(const unsigned char *input, size_t input_len,
                      unsigned char **output, size_t *output_len) {
    size_t capacity, i, o = 0, padding = 0;
    if (input_len == 0 || input_len % 4 != 0) return -1;
    if (input[input_len - 1] == '=') padding++;
    if (input_len > 1 && input[input_len - 2] == '=') padding++;
    capacity = (input_len / 4) * 3 - padding;
    *output = malloc(capacity + 1);
    if (!*output) return -1;
    for (i = 0; i < input_len; i += 4) {
        int a = base64_value(input[i]);
        int b = base64_value(input[i + 1]);
        int c = input[i + 2] == '=' ? 0 : base64_value(input[i + 2]);
        int d = input[i + 3] == '=' ? 0 : base64_value(input[i + 3]);
        int final = i + 4 == input_len;
        unsigned int value;
        if (a < 0 || a > 63 || b < 0 || b > 63 || c < 0 || c > 63 || d < 0 || d > 63 ||
            (input[i + 2] == '=' && (!final || input[i + 3] != '=')) ||
            (input[i + 3] == '=' && !final) ||
            (input[i + 2] == '=' && (b & 15) != 0) ||
            (input[i + 3] == '=' && input[i + 2] != '=' && (c & 3) != 0)) {
            free(*output); *output = NULL; return -1;
        }
        value = ((unsigned int)a << 18) | ((unsigned int)b << 12) |
                ((unsigned int)c << 6) | (unsigned int)d;
        if (o < capacity) (*output)[o++] = (unsigned char)(value >> 16);
        if (o < capacity) (*output)[o++] = (unsigned char)(value >> 8);
        if (o < capacity) (*output)[o++] = (unsigned char)value;
    }
    *output_len = o;
    (*output)[*output_len] = 0;
    return 0;
}

int idc_sha256_hex(const unsigned char *data, size_t len, char output[65]) {
    uint32_t state[8] = { 0x6a09e667U,0xbb67ae85U,0x3c6ef372U,0xa54ff53aU,
                          0x510e527fU,0x9b05688cU,0x1f83d9abU,0x5be0cd19U };
    unsigned char tail[128];
    size_t full = len / 64, remainder = len % 64, i;
    uint64_t bit_len = (uint64_t)len * 8U;
    size_t tail_len;
    for (i = 0; i < full; i++) sha256_block(data + i * 64, state);
    if (remainder) memcpy(tail, data + full * 64, remainder);
    tail[remainder] = 0x80;
    tail_len = remainder < 56 ? 64 : 128;
    memset(tail + remainder + 1, 0, tail_len - remainder - 1);
    for (i = 0; i < 8; i++) tail[tail_len - 1 - i] = (unsigned char)(bit_len >> (i * 8));
    sha256_block(tail, state);
    if (tail_len == 128) sha256_block(tail + 64, state);
    for (i = 0; i < 8; i++)
        snprintf(output + i * 8, 9, "%08x", state[i]);
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
        char *slash = strrchr(directories[i], '/');
        char root[128];
        size_t root_len;
        if (!slash) continue;
        root_len = (size_t)(slash - directories[i]);
        if (root_len >= sizeof(root)) continue;
        memcpy(root, directories[i], root_len);
        root[root_len] = '\0';
        if (stat(root, &st) != 0 || !S_ISDIR(st.st_mode)) continue;
        if (stat(directories[i], &st) != 0 && mkdir(directories[i], 0700) != 0) continue;
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
