#include "kv.h"
#include "util.h"
#include <assert.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define CMD_BUFFER_SIZE 256

static volatile bool running = true;

void int_handler() {
  puts("bye bye!");
  running = false;
}

int main(void) {
  signal(SIGINT, int_handler);
  puts("=========== my key-value store 0.0.1 ===========");
  char cmd[CMD_BUFFER_SIZE] = {0};
  kv_store kv = kv_init("my-kv");
  while (true) {
    printf("> ");
    read_line(cmd, CMD_BUFFER_SIZE, stdin);
    if (!running)
      break;
    char *arg1 = strtok(cmd, " ");
    char *arg2 = strtok(NULL, " ");
    char *arg3 = strtok(NULL, " ");
    assert(arg1);
    if (strcmp(arg1, "get") == 0 && arg2) {
      buffer out;
      if (kv_get(&kv, arg2, &out)) {
        printf("%s\n", out.data);
      } else {
        puts("nil");
      }
    } else if (strcmp(arg1, "put") == 0 && arg2) {
      if (arg3 == NULL) {
        puts("bad cmd! value is missing");
        continue;
      }
      kv_put(&kv, arg2, arg3);
    } else if (strcmp(arg1, "del") == 0 && arg2) {
      if (kv_del(&kv, arg2)) {
        puts("OK");
      } else {
        puts("nil");
      }
    } else if (strcmp(arg1, "dbg") == 0) {
      kv_dbg(&kv);
    } else {
      puts("Invalid command!");
    }
  }
  kv_close(&kv);
}
