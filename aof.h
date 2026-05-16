#ifndef AOF_H
#define AOF_H

#include "util.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

typedef struct aof {
  FILE *file;
  const char *path;
} aof;

enum operation_type {
  PUT,
  DEL,
};

typedef struct put_op {
  char *key;
  char *value;
} put_op;

typedef struct del_op {
  char *key;
} del_op;

union operation {
  put_op put;
  del_op del;
};

typedef struct log {
  union operation op;
  enum operation_type type;
} log;

aof aof_init(const char *path);
void aof_append(aof *f, log l);
void aof_close(aof *f);
void aof_foreach(aof *f, void (*on_each)(log *, void *), void *ctx);

aof aof_init(const char *path) {
  FILE *f = fopen(path, "a");
  if (f == NULL) {
    perror("Error opening the file");
    return (aof){0};
  }
  return (aof){
      .file = f,
      .path = path,
  };
}

void aof_append(aof *f, log l) {
  assert(f);
  char content[256];
  switch (l.type) {
  case PUT:
    sprintf(content, "PUT:%s:%s", l.op.put.key, l.op.put.value);
    break;
  case DEL:
    sprintf(content, "DEL:%s", l.op.del.key);
    break;
  }

  fprintf(f->file, "%s\n", content);
}

void aof_close(aof *f) {
  assert(f);
  if (f->file) {
    fclose(f->file);
  }
}

void aof_foreach(aof *f, void (*on_each)(log *, void *), void *ctx) {
  assert(f);
  FILE *fr = fopen(f->path, "r");
  if (fr == NULL) {
    perror("Error opening the file");
    return;
  }
  while (true) {
    char line[256] = {0};
    read_line(line, 256, fr);
    if (line[0] == '\0') {
      fclose(fr);
      return;
    }
    char *op = strtok(line, ":");
    char *key = strtok(NULL, ":");
    char *val = strtok(NULL, ":");
    log out = {0};
    if (strcmp(op, "PUT") == 0 && key && val) {
      out.type = PUT;
      out.op.put.key = key;
      out.op.put.value = val;
    } else if (strcmp(op, "DEL") == 0 && key) {
      out.type = DEL;
      out.op.del.key = key;
    }
    on_each(&out, ctx);
  }
}

#endif
