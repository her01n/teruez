#include <assert.h>
#include <errno.h>

#include "uri.h"

int fromHexDigit(char digit) {
    if (digit >= '0' && digit <= '9') {
        return digit - '0';
    } else if (digit >= 'a' && digit <= 'f') {
        return 10 + digit - 'a';
    } else if (digit >= 'A' && digit <= 'F') {
        return 10 + digit - 'A';
    } else {
        return -1;
    }
}

int unescapeURI(char* uri) {
    char* src = uri;
    char* dest = uri;
    while (*src) {
        if (*src == '%') {
            if (src[1] == 0 || src[2] == 0) {
                return EINVAL;
            }
            int ms = fromHexDigit(src[1]);
            int ls = fromHexDigit(src[2]);
            if (ms < 0 || ls < 0) {
                errno = EINVAL;
                return -1;
            }
            *dest = ms*16 + ls;
            dest++;
            src += 3;
        } else {
            *dest = *src;
            dest++;
            src++;
        }
    }
    *dest = 0;
    return 0;
}

