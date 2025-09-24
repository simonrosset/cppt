#ifndef SERVER_H
#define SERVER_H

typedef struct {
    int port;
    char root[256];
    char logdir[256];
} server_config_t;

extern server_config_t cfg;   // globale Konfig sichtbar machen

int load_config(const char *filename, server_config_t *config);
int server_init(int port);
void server_run(void);
void server_cleanup(void);
void write_access_log(const char *client_ip, const char *method, const char *path, int status);

#endif