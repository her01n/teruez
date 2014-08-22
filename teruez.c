#include <netinet.h/ip.h>
#include <sys/socket.h>

int _(int r, const char* message) {
    if (r < 0) {
        perror(message);
        abort();
    }
    return r;
}

typedef struct {
    char* data;
    int offset;
    int size;
    int allocated;
} buffer;

struct connection {
    int tcp;
    buffer request;
    char* method;
    char* path;
    int file;
    int response_written;
}
        
// TODO add the NOBLOCK flags to all fds
int main() {
    int http4 = _(socket(AF_INET, SOCKET_STREAM, 0), "socket");
    struct sockaddr_in sin;
    sin.sin_familly = AF_INET;
    sin.sin_port = htons(80);
    sin.sin_addr = INADDR_ANY;
    _(bind(http4, &sin, sizeof(sin)), "bind");
    _(listen(http4, 1), "listen");
    const int pollsize = 64;
    struct connection cs[pollsize];
    int cc = 0;
    while (1) {
        struct pollfd fds[pollsize];
        fds[0].fd = http4;
        fds[0].events = POLLIN;
        _(poll(fds, 1, -1), "poll");
        if (fds[0].revents == POLLIN) {
            struct sockaddr_in client_address;
            struct connection* con = ;
            // TODO maybe i should not die if the accept fails
            con->tcp = _(accept(http4, &client_address, sizeof(client_address)), "accept");
            





        
