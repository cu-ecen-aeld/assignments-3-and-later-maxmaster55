// #define NDEBUG // disables assert()
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <syslog.h>
#include <unistd.h>

#define FILE_NAME "/var/tmp/aesdsocketdata"
#define PORT "9000"
#define DATA_LIMIT (20242)
FILE *file;
int server;

struct addrinfo hints;
struct addrinfo *address_info;

void sig_handler(int sig_num) {
  if (sig_num == SIGINT || sig_num == SIGTERM) {
    close(server);
    fclose(file);
    freeaddrinfo(address_info);
    remove(FILE_NAME);
  }
  exit(0);
}

int main(int argc, char *argv[]) {
  int pid = 1;
  if (argc == 2) {
    if (strcmp(argv[1], "-d") == 0) {
      printf("Running as a daemon\n");
      pid = fork();
      if (pid != 0) {
        return 0;
      }

      int sid = setsid();
      if (sid < 0) {
        exit(EXIT_FAILURE);
      }

      if (chdir("/") < 0) {
        exit(EXIT_FAILURE);
      }
    }
  }

  // init file
  if (access(FILE_NAME, F_OK) == 0) {
    remove(FILE_NAME);
  }

  // setup logging
  openlog("aesdserver", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
  // register signals
  signal(SIGINT, sig_handler);
  signal(SIGTERM, sig_handler);

  server = socket(AF_INET, SOCK_STREAM, 0);
  if (server == -1) {
    perror("Error creating socket");
    exit(EXIT_FAILURE);
  }
  if (pid == 0) {
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
  }

  // jsut to make sure there is no garbage data
  memset(&hints, 0, sizeof(hints));

  hints.ai_flags = AI_PASSIVE;
  if (getaddrinfo(NULL, PORT, &hints, &address_info) != 0) {
    perror("Error with getaddrinfo");
    exit(EXIT_FAILURE);
  }

  if (bind(server, address_info->ai_addr, address_info->ai_addrlen) != 0) {
    perror("Error with bind");
    exit(EXIT_FAILURE);
  }

  if (listen(server, 20) != 0) {
    perror("Error with listen");
    exit(EXIT_FAILURE);
  }

  printf("waiting for connection on port is %s\n", PORT);
  fflush(stdout);

  int confd;
  struct sockaddr addr;
  while (true) {
    file = fopen(FILE_NAME, "a+");
    socklen_t addr_len = sizeof(addr);

    memset(&addr, 0, sizeof(addr));
    confd = accept(server, &addr, &addr_len);

    // all of this just to get the address of the connected client
    struct sockaddr_in *pV4Addr = (struct sockaddr_in *)&addr;
    struct in_addr ipAddr = pV4Addr->sin_addr;
    char client_addr_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &ipAddr, client_addr_str, INET_ADDRSTRLEN);

    syslog(LOG_DEBUG, "Accepted connection from %s", client_addr_str);
    printf("Accepted connection from %s\n", client_addr_str);

    char *buff = (char *)malloc(DATA_LIMIT);
    if (buff == NULL) {
      perror("error allocating memory");
      exit(EXIT_FAILURE);
    }
    memset(buff, 0, DATA_LIMIT);
    int re = recv(confd, buff, DATA_LIMIT, 0);
    if (re > 0) {

      // save buffer to the file
      fprintf(file, "%s", buff);

      // get file size
      fseek(file, 0, SEEK_END);
      long file_size = ftell(file);
      fseek(file, 0, SEEK_SET);

      fread(buff, 1, file_size, file);

      send(confd, buff, strlen(buff), 0);

      close(confd);
      fclose(file);
      syslog(LOG_DEBUG, "Closed connection from %s", client_addr_str);
    }
    free(buff);
  }
  freeaddrinfo(address_info);
  return EXIT_SUCCESS;
}
