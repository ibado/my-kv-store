#include "hashtable.h"
#include <assert.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CMD_BUFFER_SIZE 256

static volatile bool running = true;

void int_handler() {
  puts("bye bye!");
  running = false;
}

void read_stdin(char *out, size_t len) {
  char *str_read = fgets(out, len, stdin);
  if (!str_read)
    return;

  int i = 0;
  while (out[i] != '\n' && out[i] != '\0')
    i++;

  if (out[i] == '\n')
    out[i] = '\0';
}

int main(void) {
  signal(SIGINT, int_handler);
  puts("=========== my key-value store 0.0.1 ===========");
  char cmd[CMD_BUFFER_SIZE] = {0};
  hash_table ht = ht_init();
  while (true) {
    printf("> ");
    read_stdin(cmd, CMD_BUFFER_SIZE);
    if (!running)
      break;
    char *arg1 = strtok(cmd, " ");
    char *arg2 = strtok(NULL, " ");
    char *arg3 = strtok(NULL, " ");
    assert(arg1);
    assert(arg2); // TODO: print nice message instead
    if (strcmp(arg1, "get") == 0) {
      buffer out;
      if (ht_get(&ht, arg2, &out)) {
        printf("%s\n", out.data);
      } else {
        puts("nil");
      }
    } else if (strcmp(arg1, "put") == 0) {
      if (arg3 == NULL) {
        puts("bad cmd! value is missing");
        continue;
      }
      ht_put(&ht, arg2, arg3);
    } else if (strcmp(arg1, "del") == 0) {
      if (ht_del(&ht, arg2))
        puts("OK");
      else
        puts("nil");
    } else {
      puts("Invalid command!");
    }
  }
  ht_free(&ht);
}
