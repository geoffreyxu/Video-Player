#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <sys/socket.h>

/**
 * Project 1 starter code
 * All parts needed to be changed/added are marked with TODO
 */

#define BUFFER_SIZE 1024
#define DEFAULT_SERVER_PORT 8081
#define DEFAULT_REMOTE_HOST "131.179.176.34"
#define DEFAULT_REMOTE_PORT 5001

struct server_app {
    // Parameters of the server
    // Local port of HTTP server
    uint16_t server_port;

    // Remote host and port of remote proxy
    char *remote_host;
    uint16_t remote_port;
};

// The following function is implemented for you and doesn't need
// to be change
void parse_args(int argc, char *argv[], struct server_app *app);

// The following functions need to be updated
void handle_request(struct server_app *app, int client_socket);
void serve_local_file(int client_socket, const char *path);
void proxy_remote_file(struct server_app *app, int client_socket, const char *path);

// The main function is provided and no change is needed
int main(int argc, char *argv[])
{
    struct server_app app;
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    int ret;

    parse_args(argc, argv, &app);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(app.server_port);

    // The following allows the program to immediately bind to the port in case
    // previous run exits recently
    int optval = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 10) == -1) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", app.server_port);

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket == -1) {
            perror("accept failed");
            continue;
        }
        
        printf("Accepted connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        handle_request(&app, client_socket);
        close(client_socket);
    }

    close(server_socket);
    return 0;
}

void parse_args(int argc, char *argv[], struct server_app *app)
{
    int opt;

    app->server_port = DEFAULT_SERVER_PORT;
    app->remote_host = NULL;
    app->remote_port = DEFAULT_REMOTE_PORT;

    while ((opt = getopt(argc, argv, "b:r:p:")) != -1) {
        switch (opt) {
        case 'b':
            app->server_port = atoi(optarg);
            break;
        case 'r':
            app->remote_host = strdup(optarg);
            break;
        case 'p':
            app->remote_port = atoi(optarg);
            break;
        default: /* Unrecognized parameter or "-?" */
            fprintf(stderr, "Usage: server [-b local_port] [-r remote_host] [-p remote_port]\n");
            exit(-1);
            break;
        }
    }

    if (app->remote_host == NULL) {
        app->remote_host = strdup(DEFAULT_REMOTE_HOST);
    }
}

void handle_request(struct server_app *app, int client_socket) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    // Read the request from HTTP client
    // Note: This code is not ideal in the real world because it
    // assumes that the request header is small enough and can be read
    // once as a whole.
    // However, the current version suffices for our testing.
    bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read <= 0) {
        return;  // Connection closed or error
    }

    buffer[bytes_read] = '\0';
    // copy buffer to a new string
    char *request = malloc(strlen(buffer) + 1);
    strcpy(request, buffer);
    //printf("%s\n", request);
    // TODO: Parse the header and extract essential fields, e.g. file name
    // Hint: if the requested path is "/" (root), default to index.html
    char *get = strtok(request, " ");      
    char *file_name = strtok(NULL, " ");        
    char *version = strtok(NULL, "\n");       

    char *clean_ptr = file_name;
    while (*clean_ptr != '\0') {
        if (strncmp(clean_ptr, "%20", 3) == 0) {
            *clean_ptr = ' ';
            memmove(clean_ptr + 1, clean_ptr + 3, strlen(clean_ptr + 3) + 1);
        }
        else if (strncmp(clean_ptr, "%25", 3) == 0) {
            *clean_ptr = '%';
            memmove(clean_ptr + 1, clean_ptr + 3, strlen(clean_ptr + 3) + 1);
        }
        clean_ptr++;
    }
    //printf("REQUEST: %s\n", request);
    //printf("LINE: %s\n", get);
    //printf("VERSION: %s\n", version);
    //printf("FILE: %s\n", file_name);

    if (strcmp(get, "GET") != 0) {
        printf("Not a GET request");
        return;
    }
    if (strcmp(file_name, "/") == 0) {
        file_name = "index.html";
    }
    else {
        file_name++;
    }
    // TODO: Implement proxy and call the function under condition
    // specified in the spec
    // if (need_proxy(...)) {
    //    proxy_remote_file(app, client_socket, file_name);
    // } else {
    //printf("%s\n", file_name);
    //char file_ext[] = ".ts";
    //size_t name_len = strlen(file_name);
    //size_t ext_len = strlen(file_ext);

    //printf("%s\n", file_name);
    //printf("%s\n", file_name + name_len - ext_len);
    //printf("%s\n", file_ext);
    //printf("%d\n", strcmp(file_name + name_len - ext_len, file_ext));

    char *file_ext = strrchr(file_name, '.');
    if (file_ext != NULL && strcmp(file_ext, ".ts") == 0) {
        //printf("proxy\n");
        proxy_remote_file(app, client_socket, buffer);
    }
    else {
        //printf("local\n");
        serve_local_file(client_socket, file_name);
    }
    //}
}

void serve_local_file(int client_socket, const char *path) {
    // TODO: Properly implement serving of local files
    // The following code returns a dummy response for all requests
    // but it should give you a rough idea about what a proper response looks like
    // What you need to do 
    // (when the requested file exists):
    // * Open the requested file
    // * Build proper response headers (see details in the spec), and send them
    // * Also send file content
    // (When the requested file does not exist):
    // * Generate a correct response

    char response_header[BUFFER_SIZE]; // Adjust the buffer size as needed
    char *response_body = NULL;
    ssize_t response_size = 0;
    FILE *fp;
    int file_fd;
    int file_size;


    // Open the requested file
    fp = fopen(path, "r");
    if (fp == NULL) {
        // Handle the case when the file does not exist
        char not_found_response[] = "HTTP/1.1 404 Not Found\r\n"
                                   "Content-Type: text/plain\r\n"
                                   "\r\n"
                                   "File Not Found";
        send(client_socket, not_found_response, strlen(not_found_response), 0);
        //fclose(fp);
        return;
    }

    //Get file type
    char *file_ext = strrchr(path, '.');
    char *content_type = "application/octet-stream";
    if(file_ext == NULL){
        ;
    }
    else if(strcmp(file_ext + 1, "html") == 0){
        content_type = "text/html; charset=UTF-8";
    }
    else if(strcmp(file_ext + 1, "txt") == 0){
        content_type = "text/plain; charset=UTF-8";
    }
    else if(strcmp(file_ext + 1, "jpg") == 0){
        content_type = "image/jpeg";
    }

    // Calculate the size of the file
    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    
    // Build response headers
    snprintf(response_header, sizeof(response_header),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %d\r\n"
             "\r\n",
             content_type, file_size);
 
    // Send response headers
    send(client_socket, response_header, strlen(response_header), 0);
    //printf("%s", response_header);

    // Send the file content
    fseek(fp, 0, SEEK_SET);
    response_body = (char *)malloc(BUFFER_SIZE);
    ssize_t nbytes = 0;
    while ((nbytes = fread(response_body, 1, BUFFER_SIZE, fp)) > 0) {
        ssize_t bytes_sent = send(client_socket, response_body, nbytes, 0);
        if (bytes_sent < 0) {
            perror("send failed");
            break;
        }
        response_size += bytes_sent;
    }

    // Clean up
    free(response_body);
    fclose(fp);
}

void proxy_remote_file(struct server_app *app, int client_socket, const char *request) {
    // TODO: Implement proxy request and replace the following code
    // What's needed:
    // * Connect to remote server (app->remote_server/app->remote_port)
    // * Forward the original request to the remote server
    // * Pass the response from remote server back
    // Bonus:
    // * When connection to the remote server fail, properly generate
    // HTTP 502 "Bad Gateway" response


    int remote_socket = socket(AF_INET, SOCK_STREAM, 0);
    char buffer[BUFFER_SIZE];
    int bytes_read;

    if (remote_socket < 0) {
        perror("Socket Failure");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in remote_addr;
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(app->remote_port);
    remote_addr.sin_addr.s_addr = inet_addr(app->remote_host);

    if (connect(remote_socket, (struct sockaddr *)&remote_addr, sizeof(remote_addr)) < 0) {
        char response[] = "HTTP/1.0 502 Bad Gateway\r\n\r\n";
        send(client_socket, response, strlen(response), 0);
        close(remote_socket);
        return;
    }
    
    // Send the original request to the remote server
    int nbytes = send(remote_socket, request, strlen(request), 0);
    //printf("NBYTES: %d\n", nbytes);
    // Receive the response from the remote server
    while ((bytes_read = recv(remote_socket, buffer, sizeof(buffer) - 1, 0)) > 0) {
        // Forward the response to the client
        send(client_socket, buffer, bytes_read, 0);
    //printf("%d\n", bytes_read);
}

    if (bytes_read < 0) {
        perror("recv failed");
    }

    // Close both sockets
    close(remote_socket);
    close(client_socket);
    

    //send(client_socket, response, strlen(response), 0);
}