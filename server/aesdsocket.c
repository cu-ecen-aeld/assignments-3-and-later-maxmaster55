#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

#define FILE_PATH "/var/tmp/aesdsocketdata"
#define BUFF_SIZE 1024
#define MAX_CONNECTED 10
#define PORT 9000

int is_daemon = false;
int server_socket;

void handler(int SIG);

int main(int argc, char *argv[]) {
  if (argc == 2) {
    if (strcmp(argv[1], "-d") == 0) {
      printf("Running in daemon mode\n");
      if (fork() != 0) {
        is_daemon = true;
        exit(EXIT_SUCCESS);
      }
    }
  }

  // start socket
  char buf[BUFF_SIZE];
  int connfd;
  int len;
  struct sockaddr_in servaddr_in, cli_in;
  server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket < 0) {
    perror("Error with the socket");
    exit(EXIT_FAILURE);
  }

  memset(&servaddr_in, 0, sizeof(struct sockaddr_in));
  memset(&cli_in, 0, sizeof(struct sockaddr_in));

  int opt = 1;
  int rc =
      setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  servaddr_in.sin_family = AF_INET;
  servaddr_in.sin_addr.s_addr = INADDR_ANY;
  servaddr_in.sin_port = htons(PORT);

  if ((bind(server_socket, (struct sockaddr *)&servaddr_in,
            sizeof(servaddr_in))) != 0) {
    perror("Error when binding socket");
    return -1;
  }
  struct sigaction sigint;
  memset(&sigint, 0, sizeof(struct sigaction));
  sigint.sa_handler = handler;
  sigaction(SIGINT, &sigint, NULL);
  sigaction(SIGTERM, &sigint, NULL);

  for (;;) {

    if ((listen(server_socket, 5)) != 0) {
      perror("Listen failed");
      return -1;
    }
    len = sizeof(cli_in);

    connfd = accept(server_socket, (struct sockaddr *)&cli_in, &len);
    if (connfd < 0) {
      perror("server accept failed");
      return -1;
    } else {
      // https://stackoverflow.com/questions/3060950/how-to-get-ip-address-from-sock-structure-in-c
      char str[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &cli_in.sin_addr, str, INET_ADDRSTRLEN);
      syslog(LOG_USER | LOG_INFO, "Accepted connection from %s", str);
    }

    FILE *fi = fopen(FILE_PATH, "a+");
    if (fi == NULL) {
      perror("Could not open file");
      return -1;
    }

    for (;;) {
      memset(buf, 0, BUFF_SIZE);

      int ret;
      ret = recv(connfd, &buf, BUFF_SIZE, 0);
      if (ret < 0) {
        perror("Error when reading socket data");
        return -1;
      }

      fwrite(buf, 1, ret, fi);

      if (buf[ret - 1] == '\n') {
        break;
      }
    }

    fseek(fi, 0, SEEK_END);
    size_t fsize = ftell(fi);
    if (fseek(fi, 0, SEEK_SET) == -1) {
      perror("Failed to set fd to beginning of file");
      return -1;
    }
    memset(buf, 0, BUFF_SIZE);

    char *fbuf[fsize];
    memset(fbuf, 0, fsize);
    int ret = fread(fbuf, 1, fsize, fi);

    if (ret < 0) {
      perror("Failed reading file to send back to client");
      return -1;
    } else {
      send(connfd, fbuf, fsize, 0);
      memset(buf, 0, BUFF_SIZE);
      memset(fbuf, 0, fsize);
    }

    fflush(fi);
    if (fclose(fi) < 0) {
      perror("Error when closing file");
      return -1;
    }

    if (close(connfd) < 0) {
      perror("Error when closing connection");
      return -1;
    }
  }

  return 0;
}

void handler(int SIG) {
  close(server_socket);
  remove(FILE_PATH);
  exit(0);
}
