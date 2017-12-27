int create_sock();
void set_setreuse(int sock, int reuse);
void set_setreuse_port(int sock, int reuse);
void sock_bind(int sock, int port);
void sock_listen(int sock, int port, int maxconn);
int server_init(int port, int maxconn);
