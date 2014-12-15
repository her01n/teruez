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

int unescapeURI(char* dest, char* src, int size) {
    assert(size > 0);
    int i = 0;
    while (*src) {
        if (i == size) {
            errno = ENOMEM;
            return -1;
        }
        if (*src == '%') {
            if (src[1] == 0 || src[2] == 0) {
                errno = EINVAL;
                return -1;
            }
            int ms = fromHexDigit(src[1]);
            int ls = fromHexDigit(src[2]);
            if (ms < 0 || ls < 0) {
                errno = EINVAL;
                return -1;
            }
            dest[i] = ms*16 + ls;
            src += 3;
        } else {
            dest[i] = *src;
            src++;
        }
        i++;
    }
    dest[i] = 0;
    return i;
}

