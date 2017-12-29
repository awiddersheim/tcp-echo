#include "main.h"

int create_sock()
{
    int sock;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        log_fatal("Could not create socket");

    return sock;
}

void sock_setreuse(int sock, int reuse)
{
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1)
        log_errno("Could not set address reuse");
}

void sock_setreuse_port(int sock, int reuse)
{
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) == -1)
        log_errno("Could not set port reuse");
}

void sock_bind(int sock, int port)
{
    struct sockaddr_in addr;

    memset(&addr, 0x0, sizeof(struct sockaddr_in));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        log_errno("Could not bind to port (%d)", port);
}

void sock_listen(int sock, int maxconn, int port)
{
    if (listen(sock, maxconn) < 0)
        log_errno("Could not listen on port (%d)", port);
}

int server_init(int port, int maxconn)
{
    int sock;

    sock = create_sock();

    sock_setreuse(sock, 1);
    sock_setreuse_port(sock, 1);

    sock_bind(sock, port);
    sock_listen(sock, maxconn, port);

    return sock;
}
