#ifndef AOF
#define AOF

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

typedef struct aof_it {
  FILE *f;
} aof_it;

aof aof_init(const char *path);
void aof_append(aof *f, log l);
void aof_close(aof *f);

aof_it aof_it_new(aof *f);
bool aof_it_next(aof_it *fit, log *out);

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

aof_it aof_it_new(aof *f) {
  assert(f);
  FILE *fr = fopen(f->path, "r");
  if (fr == NULL) {
    perror("Error opening the file");
    return (aof_it){0};
  }
  return (aof_it){.f = fr};
}

bool aof_it_next(aof_it *fit, log *out) {
  if (fit->f == NULL)
    return false;
  char line[256] = {0};
  read_line(line, 256, fit->f);
  if (strlen(line) == 0) {
    fclose(fit->f);
    fit->f = NULL;
    return false;
  }
  char *op = strtok(line, ":");
  char *key = strtok(NULL, ":");
  char *val = strtok(NULL, ":");
  if (strcmp(op, "PUT") == 0 && key && val) {
    out->type = PUT;
    out->op.put.key = key;
    out->op.put.value = val;
  } else if (strcmp(op, "DEL") == 0 && key) {
    out->type = DEL;
    out->op.del.key = key;
  }
  return true;
}

#endif
