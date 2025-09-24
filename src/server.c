#include <stdio.h>
#include <string.h> // memset
#include <unistd.h> // close()
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> // sockaddr_in
#include <arpa/inet.h>  // htons, htonl
#include <sys/stat.h>
#include <time.h>    // timestamp for logging
#include <signal.h>  // handle signals like CTRL+C
#include <pthread.h> // for threading
#include <stdlib.h> // malloc, free, atoi


// own headers
#include "server.h"

void* handle_client(void *arg);
static int server_fd = -1; // save gloval server socket file descriptor
static int running = 1;    // läuft, solange = 1
server_config_t cfg;       // globale Config
 // define a structure to hold client information
        struct client_info
        {
            int fd;
            char ip[INET_ADDRSTRLEN];
        };

void handle_signal(int sig)
{
    running = 0; // terminate the server loop
    printf("signal %d received, server terminates...\n", sig);
    _exit(0); // exit immediately
}

int load_config(const char *filename, server_config_t *config)
{
    // open config file
    FILE *f = fopen(filename, "r");
    if (!f)
    {
        perror("fopen config");
        return -1;
    }

    char line[512];
    // get each line of the config file
    while (fgets(line, sizeof(line), f))
    {
        char key[128], value[256];
        if (sscanf(line, "%127[^=]=%255s", key, value) == 2)
        {
            if (strcmp(key, "port") == 0)
            {
                config->port = atoi(value);
            }
            else if (strcmp(key, "root") == 0)
            {
                strncpy(config->root, value, sizeof(config->root) - 1);
            }
            else if (strcmp(key, "logdir") == 0)
            {
                strncpy(config->logdir, value, sizeof(config->logdir) - 1);
            }
        }
    }

    fclose(f);
    return 0;
}

int server_init(int port)
{

    // Create a TCP socket on global variable
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    // Check for errors, return -1 on error and print error message for debugging.
    if (server_fd < 0)
    {
        perror("socket");
        return -1;
    }
    // test message
  //  printf("Socket erstellt: fd = %d\n", server_fd);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));           // set all bytes to 0 as else it may contain garbage values.
    addr.sin_family = AF_INET;                // IPv4, don't like ipv6 :) (Way to long to write them out for DNS)
    addr.sin_addr.s_addr = htonl(INADDR_ANY); // 0.0.0.0 (alle Interfaces) => Allow connections from any IP address
    // Check for invalid port numbers
    if (port < 0)
    {
        printf("Fehler: Ungültiger Port %d\n", port);
        close(server_fd);
        server_fd = -1;
        return -1;
    }
    addr.sin_port = htons(port); // Port is given as parameter => Use 8080 as port if no other port is specified
    // Try to bind, if it fails return -1 and print error message for debugging.
    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        close(server_fd);
        server_fd = -1;
        return -1;
    }
    // printf("Socket gebunden an Port %d\n", port);
    // Try to listen, if it fails return -1 and print error message for debugging.
    if (listen(server_fd, 5) < 0)
    {
        perror("listen");
        close(server_fd);
        server_fd = -1;
        return -1;
    }
    //printf("Server lauscht jetzt auf Port %d\n", port);
    // Initialize successfully completed.
    signal(SIGINT, handle_signal);  // CTRL+C
    signal(SIGTERM, handle_signal); // kill
    printf("server listening on port %d\n", port);
    return 0;
}

const char *get_mime_type(const char *path)
{
    // Get correct MIME type based on file extension
    const char *ext = strrchr(path, '.');
    if (!ext)
        return "application/octet-stream"; // fallback if no extension found
    // Compare known extensions
    if (strcmp(ext, ".html") == 0)
        return "text/html";
    if (strcmp(ext, ".htm") == 0)
        return "text/html";
    if (strcmp(ext, ".css") == 0)
        return "text/css";
    if (strcmp(ext, ".js") == 0)
        return "application/javascript";
    if (strcmp(ext, ".png") == 0)
        return "image/png";
    if (strcmp(ext, ".jpg") == 0)
        return "image/jpeg";
    if (strcmp(ext, ".jpeg") == 0)
        return "image/jpeg";
    if (strcmp(ext, ".gif") == 0)
        return "image/gif";
    if (strcmp(ext, ".txt") == 0)
        return "text/plain";

    return "application/octet-stream"; // fallback for unknown types
}

void server_run(void)
{
   // puts("Warte auf eingehende Verbindung...");
    while (running)
    {

        // Adress structure for the client. Get the address of the connecting client.
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

       

        // Waiting for a connection via browser or telnet/curl etc.
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);

        // allocate memory for client info and save the file descriptor and IP address
        struct client_info *info = malloc(sizeof(struct client_info));
        info->fd = client_fd;
        // convert the IP address to a string
        inet_ntop(AF_INET, &client_addr.sin_addr, info->ip, sizeof(info->ip));

        // Check for successful accept, else print error message for debugging.
        if (client_fd < 0)
        {
            perror("accept failed");
            return;
        }

     
        pthread_t tid;

        // create a new thread to handle the client
        if (pthread_create(&tid, NULL, handle_client, info) != 0)

        {
            perror("pthread_create");
            close(client_fd);
            free(info);
        }
        else
        {
            pthread_detach(tid); // Thread automatisch aufräumen
        }
    }
}

void *handle_client(void *arg)
{
    struct client_info *info = arg;
    // Get the client file descriptor from the argument
    int client_fd = info->fd;

    // Buffer to store the client IP address
    char client_ip[INET_ADDRSTRLEN];
    // Get the client address info and free the allocated memory
    strncpy(client_ip, info->ip, sizeof(client_ip));
    free(info);
    //keep alive default to 1
    int keep_alive = 1; 
    while (keep_alive)
    {
        

    // Get the client IP address
    char buffer[8192];
    // Allocate memory and set all bytes to 0
    memset(buffer, 0, sizeof(buffer));

    // Read data from the client
    ssize_t total_read = 0;
    while (1)
    {
        ssize_t bytes_read = read(client_fd, buffer + total_read, sizeof(buffer) - total_read - 1);
        if (bytes_read <= 0)
            break; // error or connection closed
        total_read += bytes_read;
        if (strstr(buffer, "\r\n\r\n"))
            break; // end of headers
    }




    // Simple parsing of the HTTP request line
    char method[8], path[1024], version[16];
    if (sscanf(buffer, "%7s %1023s %15s", method, path, version) == 3)
    {
        // Get the client IP address from the socket and log it
        write_access_log(client_ip, method, path, 0);
        // Check for Connection: close header to disable keep-alive
         if (strstr(buffer, "Connection: close") || strstr(buffer, "connection: close")) {
            keep_alive = 0;
        }

        // Routing based on the requested path
        char filepath[2048];
        if (strcmp(path, "/") == 0)
        {
            // if root is requested, serve index.html
            snprintf(filepath, sizeof(filepath), "%s/index.html", cfg.root);
        }
        else
        {
            // if other path is requested, serve the corresponding file
            snprintf(filepath, sizeof(filepath), "%s%s", cfg.root, path);
            if (filepath[strlen(filepath) - 1] == '/')
            {
                strncat(filepath, "index.html", sizeof(filepath) - strlen(filepath) - 1);
            }
            else
            {
                // check if the path is a directory
                struct stat st;
                if (stat(filepath, &st) == 0 && S_ISDIR(st.st_mode))
                {
                    // redirect to path with trailing slash to not download file
                    char location[2048];
                    snprintf(location, sizeof(location), "%s/", path);
                    char redirect[4096];
                    // send 301 Moved Permanently response
                    snprintf(redirect, sizeof(redirect),
                             "HTTP/1.1 301 Moved Permanently\r\n"
                             "Location: %s\r\n"
                             "Content-Type: text/html\r\n"
                             "Content-Length: %zu\r\n"
                             "Connection: %s\r\n\r\n"
                             "<html><body><h1>301 Moved Permanently</h1></body></html>",
                             location), 56 , keep_alive ? "keep-alive" : "close";
                    write(client_fd, redirect, strlen(redirect));
                    write_access_log(client_ip, method, path, 301);
                    close(client_fd);
                    return NULL;
                }
            }
        }

        // Try to open the requested file
        FILE *fp = fopen(filepath, "r");
        if (!fp)
        {
            // File not found, send 404 response
            const char *notfound =
                "HTTP/1.1 404 Not Found\r\n"
                "Content-Type: text/html; charset=UTF-8\r\n"
                "Connection: close\r\n\r\n"
                "<html><body><h1>404 Not Found</h1></body></html>";
            write(client_fd, notfound, strlen(notfound));
            write_access_log(client_ip, method, path, 404);
        }
        else
        {
            // File found, read its contents
            char filebuf[8192];
            size_t n = fread(filebuf, 1, sizeof(filebuf) - 1, fp);
            fclose(fp);
            // Null-terminate the buffer to make it a valid string
            filebuf[n] = '\0';
            // Determine MIME type
            const char *mime = get_mime_type(filepath);

            char header[256];
            snprintf(header, sizeof(header),
                     "HTTP/1.1 200 OK\r\n"
                     "Content-Type: %s; charset=UTF-8\r\n"
                     "Content-Length: %zu\r\n"
                     "Connection: %s\r\n\r\n",
                     mime, n , keep_alive ? "keep-alive" : "close");

            write(client_fd, header, strlen(header));
            // send file content
            write(client_fd, filebuf, n);
            write_access_log(client_ip, method, path, 200);
         }
        }
        else {
            keep_alive = 0; // invalid request, close connection
            break;
        }
    }

    // Close the connection to the client
    close(client_fd);
    // printf("Verbindung (fd=%d) geschlossen.\n", client_fd);
    return NULL;
}

void write_access_log(const char *client_ip, const char *method, const char *path, int status)
{
    char logfile[512];
    snprintf(logfile, sizeof(logfile), "%s/access.log", cfg.logdir);
    FILE *logf = fopen(logfile, "a"); // Append mode and not write mode to not overwrite existing logs
    // Check if file opened successfully
    if (!logf)
    {
        perror("logfile");
        return;
    }

    // get timestamp
    time_t now = time(NULL);
    // Convert to local time structure
    struct tm *t = localtime(&now);

    char timestr[64];
    // Format the time as YYYY-MM-DD HH:MM:SS
    strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", t);

    // write into logfile
    fprintf(logf, "[%s] %s %s %s %d\n",
            timestr, client_ip, method, path, status);
    // Close the logfile
    fclose(logf);
    return;
}

void server_cleanup(void)
{
    // Close the socket if it was opened
    if (server_fd >= 0)
    {
        close(server_fd);
        printf("Socket %d geschlossen.\n", server_fd);
        server_fd = -1;
    }
}
