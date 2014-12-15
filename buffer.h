#ifndef buffer_h_included
#define buffer_h_included

typedef struct {
    char* data;
    int allocated;
    int offset;
    int size;
    int parsed;
    int delim;
} buffer;

buffer create_buffer();
/**
 * Read data from file description into the buffer
 * Return -1 on error, errno would be set.
 * Returns 0 on end of file, or number of bytes read from fd.
 */
int copy(buffer* dst, int fd);
/**
 * Read next request from the buffer.
 * Returns null if the request is not yet read.
 * Returns null terminated string containing one request (delimited by \r\n\r\n)
 * \r\n of the delimiter is included.
 * Valid until next call to next_request, copy or destroy
 */
char* next_request(buffer* src);
/**
 * Deallocate all the buffer data.
 */
void destroy(buffer* buffer);

#endif
