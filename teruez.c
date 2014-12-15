#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/ip.h>
#include <poll.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

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
    con->response = 0;
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
    assert(con->response == 0);
    assert(con->file == -1);
}

static void fillfds(connection* con, struct pollfd* fds) {
    if (con->tcp == -1) { // nothing to do
        fds[0].fd = -1;
        fds[0].events = 0;
        fds[1].fd = -1;
        fds[1].events = 0;
    } else if (con->response != 0) { // send out reposonse
        fds[0].fd = con->tcp;
        fds[0].events = POLLOUT;
        fds[1].fd = -1;
        fds[1].events = 0;
    } else if (con->file != -1) { // send out content
        fds[0].fd = con->tcp;
        fds[0].events = POLLOUT;
        fds[1].fd = con->file;
        fds[1].events = POLLIN;
    } else { // read request
        fds[0].fd = con->tcp;
        fds[0].events = POLLIN;
        fds[1].fd = -1;
        fds[1].events = 0;
    }
}

static const char* base = "/var/www";
static const char* index_file = "index.html";

static char* now() {
    static char buf[256];
    time_t now = time(0);
    int r = strftime(buf, 256, "%a, %d %b %Y %H:%M:%S %Z", localtime(&now));
    assert(r > 0);
    return buf;
}

static void respond(connection *con, int code, char* response, 
        char* content_type, int content_length, char* content) {
    int buffer_size = 777;
    char* buffer = (char*) malloc(buffer_size);
    int response_size = snprintf(buffer, buffer_size, 
            "%d HTTP/1.1 %s\r\n"
            "Date: %s\r\n"
            "Server: Teruez/1\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %d\r\n"
            "Connection: keep\r\n"
            "\r\n%s", code, response, now(), 
            content_type, content_length, content);
    if (response_size > buffer_size) {
        assert(0);
        response_size = buffer_size;
    }
    con->response = buffer;
    con->response_written = 0;
    con->response_size = response_size;
}

static void error_respond(connection* con, int code, char* response, char* content) {
    fprintf(stderr, "Error, Sending %d %s:%s\n", code, response, content);
    respond(con, code, response, "text/plain", strlen(content), content);
}

static void error_reset(connection* con, char* format, ...) {
    va_list arglist;
    // TODO log client address
    fprintf(stderr, "Error, tcp socket: %d\n", con->tcp);
    va_start(arglist, format);
    vfprintf(stderr, format, arglist);
    va_end(arglist);
    fprintf(stderr, "Closing connection\n");
    close(con->tcp);
    con->tcp = -1;
    destroy(&con->request);
    free(con->response);
    con->response = 0;
    con->response_written = 0;
    con->response_size = 0;
    if (con->file != -1) {
        if (close(con->file) != 0) perror("close input file");
    }
    con->file = -1;
    con->file_written = 0;
    con->file_size = 0;
}

static char* take_word(char* src) {
    while (*src) {
        if (*src == ' ' || *src == '\t') {
            while (*src && (*src == ' ' || *src == '\t')) {
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
        return;
    }
    int eof = 0; // client closed connection
    if (con->response != NULL) {
        int r = write(con->tcp, 
                con->response + con->response_written, 
                con->response_size - con->response_written);
        if (r < 0) {
            error_reset(con, "Failed sending response %s\n", strerror(errno));
        } else {
            con->response_written += r;
            if (con->response_written == con->response_size) {
                free(con->response);
                con->response = 0;
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
        int r = copy(&con->request, con->tcp);
        if (r < 0) {
            error_reset(con, "Failed to read request %s\n", strerror(errno));
        } else if (r == 0) {
            eof = 1;
        }
    }
    // if not busy serve next request
    if (con->response == NULL && con->file == -1) {
        char* request = next_request(&con->request);
        if (request == 0) {
            if (eof) {
                destroy(&con->request);
                if (close(con->tcp) != 0) perror("close tcp");
                con->tcp = -1;
            }
        } else {
            char* i = request;
            char* method = i; i = take_word(i);
            char* path = i; i = take_word(i);
            char* version = i; i = take_line(i);
            if (!*method || !*path || !*version) {
                error_respond(con, 400, "Bad Request", "Malformed Request Line");
            } else if (strcmp(version, "HTTP/1.1") != 0) {
                error_respond(con, 505, "HTTP Version Not Supported", 
                        "Only HTTP/1.1 version is supported.");
            } else if (strcmp(method, "GET") == 0) {
                // TODO unescape in place
                int filepath_size = strlen(path) + 1;
                char* filepath = (char*) malloc(filepath_size);
                int r = unescapeURI(filepath, path, filepath_size);
                if (r < 0) {
                    free(filepath);
                    perror("GET: unescapeURI");
                    error_respond(con, 400, "Bad Request", "Malformed URI");
                    return;
                }
                int source = open(filepath, O_RDONLY);
                if (source == -1) {
                    fprintf(stderr, "open path '%s' file '%s/%s': %s\n",
                            path, base, filepath, strerror(errno));
                    free(filepath);
                    error_respond(con, 404, "Not Found", "Not Found");
                    return;
                }
                struct stat stat;
                r = fstat(source, &stat);
                if (r == -1) {
                    close(source);
                    fprintf(stderr, "stat file '%s/%s': '%s'\n", 
                            base, filepath, strerror(errno));
                    free(filepath);
                    error_respond(con, 500, "Internal Server Error", "Error");
                    return;
                }
                // if is a directory, try index
                if (S_ISDIR(stat.st_mode)) {
                    int dir = source;
                    source = openat(dir, index_file, O_RDONLY);
                    close(dir);
                    if (source == -1) {
                        fprintf(stderr, "open index, path '%s' file '%s/%s%s': %s\n",
                                path, base, filepath, index_file, strerror(errno));
                        free(filepath);
                        error_respond(con, 404, "Not Found", "Index Not Found");
                        return;
                    }
                    if (fstat(source, &stat) == -1) {
                        fprintf(stderr, "stat index, path '%s' file '%s/%s/%s': %s\n",
                                path, base, filepath, index_file, strerror(errno));
                        close(source);
                        free(filepath);
                        error_respond(con, 500, "Internal Server Error", "Error");
                        return;
                    }
                }
                if (!S_ISREG(stat.st_mode)) {
                    close(source);
                    fprintf(stderr, "not a regular file: path '%s'\n", path);
                    free(filepath);
                    error_respond(con, 404, "Not Found", "Not a Regular File");
                    return;
                }
                free(filepath);
                // TODO better content type?
                respond(con, 200, "OK", "text/html", stat.st_size, "");
                con->file = source;
                con->file_written = 0;
                con->file_size = stat.st_size;
            } else if (strcmp(method, "OPTIONS") == 0) {
                respond(con, 200, "OK", "text/plain", 0, "");
            } else {
                fprintf(stderr, "Method: %s", method);
                error_respond(con, 501, "Not Implemented", "Method not Implemented");
            }
        }
    }
}

// TODO add the NOBLOCK flags to all fds
int main() {
    // chroot to base directory
    checked(chdir(base));
    checked(chroot(base));
    // listen
    int http4 = checked(socket(AF_INET, SOCK_STREAM, 0));
    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(80);
    sin.sin_addr.s_addr = INADDR_ANY;
    checked(bind(http4, (struct sockaddr*) &sin, sizeof(sin)));
    checked(listen(http4, 352));
    // initialize structs
    const int connection_count = 28;
    connection cs[connection_count];
    for (int i = 0; i != connection_count; i++) {
        connection_init(cs + i);
    }
    // serve
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
        checked(poll(fds, connection_count*2, -1));
        for (int i = 0; i != connection_count; i++) {
            if (ai == i) {
                if (fds[i*2].revents != 0) {
                    struct sockaddr_in client_address;
                    socklen_t addrlen = sizeof(client_address);
                    int tcp = accept(http4, (struct sockaddr*) &client_address, &addrlen);
                    if (tcp < 0) {
                        perror("accept ip4 http tcp connection");
                    } else {
                        connection_accept(cs + i, tcp);
                    }
                }
            } else {
                if (fds[i*2].revents != 0) {
                    if (fds[i*2 + 1].fd < 0 || fds[i*2 + 1].revents > 0) {
                        work(cs + i);
                    }
                }
            }
        }
    }
}
        
