#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "buffer.h"

static const int max_size = 64*1024;

static void check_invariants(buffer* buffer) {
    if (buffer->data == NULL) {
        assert(buffer->allocated == 0);
    } else {
        assert(buffer->allocated > 0);
    }
    assert(buffer->offset >= 0);
    assert(buffer->offset <= buffer->allocated);
    assert(buffer->size >= 0);
    assert(buffer->size <= max_size);
    assert(buffer->offset + buffer->size <= buffer->allocated);
    assert(buffer->parsed >= 0);
    assert(buffer->parsed <= buffer->size);
    assert(buffer->delim >= 0);
    assert(buffer->delim < 4);
}

buffer create_buffer() {
    buffer buffer;
    buffer.data = malloc(256); // buffer
    buffer.allocated = 256; // size of memory available
    buffer.offset = 0; // index of start of current data
    buffer.size = 0; // of current data
    buffer.parsed = 0; // current request parse so far 
    buffer.delim = 0; // index of delimeter seen
    check_invariants(&buffer);
    return buffer;
}

int copy(buffer* buffer, int fd) {
    assert(buffer->size + buffer->offset <= buffer->allocated);
    if (buffer->size + buffer->offset == buffer->allocated) {
        if (buffer->offset != 0) {
            memmove(buffer->data, buffer->data + buffer->offset, buffer->size);
            buffer->offset = 0;
        } else if (buffer->size < max_size) {
            int next_alloc;
            if (buffer->allocated < 256) {
                next_alloc = 256;
            } else {
                next_alloc = buffer->allocated*2;
            }
            buffer->data = realloc(buffer->data, next_alloc);
            if (buffer->data == NULL) return -1;
            buffer->allocated = next_alloc;
        } else {
            errno = EMSGSIZE;
            return -1;
        }
    }
    check_invariants(buffer);
    int read_index = buffer->size + buffer->offset;
    int r = read(fd, buffer->data + read_index, buffer->allocated - read_index);
    if (r < 0) return -1;
    buffer->size += r;
    check_invariants(buffer);
    return r;
}

static const char* delimiter = "\r\n\r\n";

char* next_request(buffer* buffer) {
    while (buffer->parsed < buffer->size) {
        if (buffer->data[buffer->offset + buffer->parsed] == delimiter[buffer->delim]) {
            buffer->delim++;
        } else {
            buffer->delim = 0;
        }
        buffer->parsed++;
        if (buffer->delim == 4) {
            buffer->data[buffer->parsed - 1] = 0;
            buffer->data[buffer->parsed - 2] = 0;
            char* request = buffer->data + buffer->offset;
            buffer->offset += buffer->parsed;
            buffer->size -= buffer->parsed;
            buffer->parsed = 0;
            buffer->delim = 0;
            check_invariants(buffer);
            return request;
        }
        check_invariants(buffer);
    }
    return NULL;
}

/**
 * Deallocate all the buffer data.
 */
void destroy(buffer* buffer) {
    if (buffer->data != NULL) {
        free(buffer->data);
        buffer->data = NULL;
        buffer->allocated = 0;
        buffer->size = 0;
        buffer->offset = 0;
        buffer->parsed = 0;
        buffer->delim = 0;
    }
    check_invariants(buffer);
}

