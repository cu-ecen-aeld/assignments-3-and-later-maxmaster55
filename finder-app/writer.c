#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syslog.h>
#include <syslog.h>

int main(int argc, char *argv[]) {
  openlog("writer", 0, LOG_USER);

  if (argc != 3) {
    syslog(LOG_ERR, "Invalid number of arguents: %d", argc);
    exit(1);
  }

  char *writefile = argv[1];
  char *writestr = argv[2];
  FILE *f = fopen(writefile, "w");
  if (f == NULL) {
    syslog(LOG_ERR, "Could not create file with name: %s", writefile);
    exit(1);
  }
  syslog(LOG_DEBUG, "Writing %s to %s", writestr, writestr);
  fprintf(f, "%s", writestr);

  return 0;
}
