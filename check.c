#include <stdio.h>
#include <stdlib.h>

#include "check.h"

int check_return(int rv, const char* call, const char* file, int line) {
    if (rv == -1) {
        fprintf(stderr, "Error on %s:%d\n", file, line);
        fprintf(stderr, "%s\n", call);
        perror(NULL);
        abort();
    }
    return rv;
}

