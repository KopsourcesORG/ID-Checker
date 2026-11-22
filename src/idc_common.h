#ifndef IDC_COMMON_H
#define IDC_COMMON_H

#include <stddef.h>

int idc_read_file(const char *path, unsigned char **data, size_t *len);
int idc_write_file(const char *path, const unsigned char *data, size_t len);
int idc_base64_encode(const unsigned char *input, size_t input_len,
                      unsigned char **output, size_t *output_len);
int idc_base64_decode(const unsigned char *input, size_t input_len,
                      unsigned char **output, size_t *output_len);
int idc_sha256_hex(const unsigned char *data, size_t len, char output[65]);
int idc_extract_digest(const unsigned char *decoded, size_t len, char digest[65]);
int idc_read_board_id(const char *path, char **board_id);
int idc_install_encoded_key(const char *encoded_path);

#endif
