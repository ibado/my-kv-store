#ifndef AOF
#define AOF

#include <assert.h>
#include <stdio.h>

typedef struct aof {
  FILE *file;
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

aof aof_init(const char *path) {
  FILE *f = fopen(path, "a");
  if (f == NULL) {
    perror("Error opening the file");
    return (aof){0};
  }
  return (aof){
      .file = f,
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

#endif
