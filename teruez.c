#include <netinet.h/ip.h>
#include <sys/socket.h>

#include "buffer.h"
#include "check.h"
#include "uri.h"

typedef struct { // connection
    // tcp socket
    int tcp;
    // tcp receive buffer
    buffer request;
    // response header
    char* response;
    int response_written, response_size;
    // response file reading rescriptor
    int file;
    int file_written, file_size;
} connection;

static void connection_init(connection* con) {
    con->tcp = -1;
    con->request = NULL;
    con->response = NULL;
    con->response_written = 0;
    con->response_size = 0;
    con->file = -1;
    con->file_written = 0;
    con->file_size = 0;
}

static void connection_accept(connection* con, int tcp) {
    assert(con->tcp == -1);
    con->tcp = tcp;
    con->request = create_buffer();
    assert(con->response == NULL);
    assert(con->file == -1);
}

static void fillfds(connection* con, struct pollfd* fds) {
    if (con->tcp == -1) { // nothing to do
        fds[0].fd = -1;
        fds[1].fd = -1;
    } else if (con->response != NULL) { // send out reposonse
        fds[0].fd = con->tcp;
        fds[0].events = POLLOUT;
        fds[1].fd = -1;
    } else if (con->file != -1) { // send out content
        fds[0].fd = con->tcp;
        fds[0].events = POLLOUT;
        fds[1].fd = con->file;
        fds[1].events = POLLIN;
    } else { // read request
        fds[0].fd = con->tcp;
        fds[0].events = POLLIN;
        fds[1].fd = -1;
    }
}

static const char* base = "/var/www/";
static const char* index = "index.html";

static char* now() {
    static char[256] buf;
    int r = strftime(buf, 256, "%a, %d %b %Y %H:%M:%S %Z", localtime(time(NULL)));
    assert(r > 0);
    return buf;
}

static void response(connection *con, int code, char* response, 
        char* content_type, int content_length, char* content) {
    int response_size = 501;
    char* response = (char*) malloc(response_size);
    snprintf(response, response_size, 
            "200 HTTP/1.1 OK\r\n" +
            "Date: %s\r\n" +
            "Server: Teruez/1\r\n" +
            "Content-Type: %s\r\n" +
            "Content-Length: %d\r\n" +
            "Connection: keep\r\n" +
            "\r\n%s", now(), 
            content_type, content_length, content);
    con->response = response;
    con->response_written = 0;
    con->response_size = strlen(response);
}

static void error_response(connection* con, int code, char* response, char* content) {
    response(con, code, response, "text/plain", strlen(content), content);
}

static char* take_word(char* src) {
    while (*src) {
        if (*src == ' ' || *src == '\t') {
            while (*src && (*src == ' ' || *src = '\t')) {
                *src = 0;
                src++;
            }
            return src;
        }
        src++;
    }
    return src;
}

static char* take_line(char* src) {
    while (*src) {
        if (*src == '\r' || *src == '\n') {
            while (*src && (*src == '\r' || *src == '\n')) {
                *src = 0;
                src++;
            }
            return src;
        }
        src++;
    }
    return src;
}

static void work(connection* con) {
    if (con->tcp == -1) {
        // ignored
    } else if (con->response != NULL) {
        int r = write(con->tcp, 
                con->response + con->response_written, 
                con->size - con->response_written);
        if (r < 0) {
            error_reset(con, "Failed sending response %s\n", strerror(errno));
        } else {
            con->response_written += r;
            if (con->response_written == con->response_size) {
                free(con->response);
                con->response_written = 0;
                con->response_size = 0;
            }
        }
    } else if (con->file != -1) {
        int r = sendfile(con->tcp, con->file, NULL, con->file_size - con->file_written);
        if (r < 0) {
            error_reset(con, "Failed sending content %s\n", strerror(errno));
        } else {
            con->file_written += r;
            if (con->file_written == con->file_size) {
                close(con->file);
                con->file = -1;
                con->file_written = 0;
                con->file_size = 0;
            }
        }
    } else {
        int r = copy(&con->buffer, con->tcp);
        if (r < 0) {
            error_reset(con, "Failed to read request %s\n", strerror(errno));
        } else {
            char* request = read_request(&con->buffer);
            if (request == null) {
                if (r == 0) { // closed connection
                    destroy(&con->buffer);
                    con->buffer = NULL;
                    close(con->tcp) || perror("close tcp");
                    con->tcp = -1;
                }
            } else {
                char* i = requst;
                char* method = i; i = take_word(i);
                char* path = i; i = take_work(i);
                char* version = i; i = take_line(i);
                if (!*method || !*path || !*version) {
                    error_response(con, 400, "Bad Request", "Malformed Request Line");
                } else if (strcmp(version, "HTTP/1.1") != 0) {
                    error_response(con, 505, "HTTP Version Not Supported", 
                            "Only HTTP/1.1 version is supported.");
                } else if (strcmp(method, "GET") == 0) {
                    int filepath_size = strlen(base) + strlen(path) + 1;
                    char* filepath = (char*) malloc(filepath_size);
                    strcpy(filepath, base);
                    int r = unescapeURI(base + strlen(base), path, end - i);
                    if (r < 0) {
                        error_response(con, 400, "Bad Request", "Malformed URI");
                        goto perror;
                    }
                    int source = open(filepath, O_RDONLY);
                    if (source == -1) {
                        error_response(con, 404, "Not Found", "Not Found");
                        goto perror;
                    }
                    struct stat;
                    int r = fstat(source, &stat);
                    if (r == -1) {
                        error_response(con, 500, "Internal Server Error", "Error");
                        goto perror;
                    }
                    // if is a directory, try index
                    if (S_ISDIR(stat.st_mode)) {
                        int source = openat(source, index, O_RDONLY);
                        if (source == -1) {
                            error_response(con, 404, "Not Found", "Index Not Found");
                            goto perror;
                        }
                        if (fstat(source, &stat) == -1) {
                            error_response(con, 500, "Internal Server Error", "Error");
                            goto perror;
                        }
                    }
                    if (!S_ISREG(stat.st_mode)) {
                        error_response(con, 404, "Not Found", "Not a Regular File");
                        goto error;
                    }
                    // TODO better content type?
                    response(con, 200, "OK", "text/html", stat.st_size, "");
                    con->file = source;
                    con->file_written = 0;
                    con->file_size = stat.st_mode;
                    perror:
                    perror("GET");
                    error:
                } else if (strcmp(method, "OPTIONS") == 0) {
                    int response_size = 501;
                    char* response = (char*) malloc(response_size);
                    snprintf(response, response_size,
                            "200 HTTP/1.1 OK\r\n" +
                            "Date: %s\r\n" +
                            "Server: Teruez/1\r\n" +
                            "Connection: keep\r\n" +
                            "\r\n", now());
                    con->response = response;
                    con->response_written = 0;
                    con->response_sizse = strlen(response);
                } else {
                    error_response(con, 501, "Not Implemented", "Method not Implemented");
                }
            }
        }
    }
}

/**
 * Error resolved by closing socket
 */
static void error_reset(connection* con, char* format, ...) {
    va_list arglist;
    // TODO log client address
    fprintf(stderr, "Error, tcp socket: %d\n", con->tcp);
    va_start(arglist, format);
    vfprintf(stderr, format, arglist);
    va_end(arglist);
    fprintf(stderr, "Closing connection\n");
    // TODO reset method?
    close(con->tcp);
    con->tcp = -1;
    destroy(&con->buffer);
}

// TODO add the NOBLOCK flags to all fds
int main() {
    int http4 = _(socket(AF_INET, SOCKET_STREAM, 0), "socket");
    struct sockaddr_in sin;
    sin.sin_familly = AF_INET;
    sin.sin_port = htons(80);
    sin.sin_addr = INADDR_ANY;
    checked(bind(http4, &sin, sizeof(sin)));
    checked(listen(http4, 352));
    const int connection_count = 28;
    struct connection cs[connection_count];
    for (int i = 0; i != connection_count; i++) {
        connection_init(cs + i);
    }
    while (1) {
        struct pollfd fds[connection_count*2];
        int ai = -1;
        for (int i = 0; i != connection_count; i++) {
            fillfds(cs + i, fds + i*2);
            if (ai == -1 && cs[i].tcp == -1) {
                ai = i;
                fds[i*2].fd = http4;
                fds[i*2].events = POLLIN;
            }
        }
        checked(poll(fds, fdi, -1));
        for (int i = 0; i != connection_count; i++) {
            if (ai == i) {
                struct sockaddr_in client_address;
                int tcp = accept(http4, &client_address, sizeof(client_address));
                if (tcp < 0) {
                    perror("accept ip4 http tcp connection");
                } else {
                    connection_accept(cs + i, tcp);
                }
            } else {
                work(cs + i, fds + i*2);
            }
        }
    }
}
        
