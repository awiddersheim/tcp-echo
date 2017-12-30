int create_sock();
char *key_init(int worker_id);
void set_setreuse(int sock, int reuse);
void set_setreuse_port(int sock, int reuse);
void sock_bind(int sock, int port);
void sock_listen(int sock, int port, int maxconn);
int server_init(int port, int maxconn);
sem_t *semaphore_init(char *key);
pthread_attr_t *thread_init();
