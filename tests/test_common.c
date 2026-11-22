#include "idc_common.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    static const unsigned char sample[] = "1772609505";
    static const char expected[] = "969b15d2660fad0056a89a0f87849b94343cea777dd31388c5a4a8b5c719f9bd";
    char digest[65];
    unsigned char *encoded = NULL, *decoded = NULL;
    size_t encoded_len = 0, decoded_len = 0;
    const unsigned char key[] = "IDC1\n0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\n";
    char extracted[65];

    assert(idc_sha256_hex(sample, sizeof(sample) - 1, digest) == 0);
    assert(strcmp(digest, expected) == 0);
    assert(idc_base64_encode(sample, sizeof(sample) - 1, &encoded, &encoded_len) == 0);
    assert(idc_base64_decode(encoded, encoded_len, &decoded, &decoded_len) == 0);
    assert(decoded_len == sizeof(sample) - 1);
    assert(memcmp(decoded, sample, decoded_len) == 0);
    assert(idc_extract_digest(key, sizeof(key) - 1, extracted) == 0);
    assert(strcmp(extracted, "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef") == 0);
    free(encoded);
    free(decoded);
    puts("common tests passed");
    return 0;
}
