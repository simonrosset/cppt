#include <stdio.h>
#include <string.h>   

//own
#include "server.h"

int main(void) {
    printf("starting server...\n");
    // Defaultwerte
    cfg.port = 8080;
    strcpy(cfg.root, "htdocs");
    strcpy(cfg.logdir, "log");

    if (load_config("conf/server.conf", &cfg) == 0) {
 //       printf("Config geladen: Port=%d, Root=%s, Logdir=%s\n",
      //         cfg.port, cfg.root, cfg.logdir);
    }
    else {
        printf("No config found, using default values.\n");
    }

    if (server_init(cfg.port) != 0) {
   //     fprintf(stderr, "Fehler: server_init\n");
        return 1;
    }

    server_run();
    server_cleanup(); //STands for close all connections and free resources

    return 0;
}
