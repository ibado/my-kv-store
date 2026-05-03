#include "hashtable.h"
#include "util.h"
#include "aof.h"
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
  hash_table ht = ht_init();
  aof f = aof_init("log.txt");
  aof_it it = aof_it_new(&f);
  while (true) {
    log l;
    bool ok = aof_it_next(&it, &l);
    if (!ok) break;
    switch (l.type) {
    case PUT:
      ht_put(&ht, l.op.put.key, l.op.put.value);
      break;
    case DEL:
      ht_del(&ht, l.op.del.key);
      break;
    }
  }
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
      if (ht_get(&ht, arg2, &out)) {
        printf("%s\n", out.data);
      } else {
        puts("nil");
      }
    } else if (strcmp(arg1, "put") == 0 && arg2) {
      if (arg3 == NULL) {
        puts("bad cmd! value is missing");
        continue;
      }
      ht_put(&ht, arg2, arg3);
      log l = {.type = PUT, .op = {.put = {.key = arg2, .value = arg3}}};
      aof_append(&f, l);
    } else if (strcmp(arg1, "del") == 0 && arg2) {
      if (ht_del(&ht, arg2)) {
        log l = {.type = DEL, .op = {.del = {.key = arg2}}};
        aof_append(&f, l);
        puts("OK");
      } else {
        puts("nil");
      }
    } else if (strcmp(arg1, "dbg") == 0) {
      ht_dbg(&ht);
    } else {
      puts("Invalid command!");
    }
  }
  aof_close(&f);
  ht_free(&ht);
}
