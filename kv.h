#ifndef KV_H
#define KV_H

#include "aof.h"
#include "hashtable.h"
#include <stdbool.h>

typedef struct kv_store {
  hash_table ht;
  aof f;
} kv_store;

void _fill_ht(log *l, void *ctx) {
  hash_table *ht = ctx;
  switch (l->type) {
  case PUT:
    ht_put(ht, l->op.put.key, l->op.put.value);
    break;
  case DEL:
    ht_del(ht, l->op.del.key);
    break;
  }
}

kv_store kv_init(char *name);
void kv_put(kv_store *kv, char *key, char *value);
bool kv_del(kv_store *kv, char *key);
bool kv_get(kv_store *kv, char *key, buffer *out);
void kv_dbg(kv_store *kv);
void kv_close(kv_store *kv);

kv_store kv_init(char *name) {
  kv_store kv = {
      .f = aof_init(name),
      .ht = ht_init(),
  };

  aof_foreach(&kv.f, _fill_ht, &kv.ht);
  return kv;
}

void kv_put(kv_store *kv, char *key, char *value) {
  ht_put(&kv->ht, key, value);
  log l = {.type = PUT, .op = {.put = {.key = key, .value = value}}};
  aof_append(&kv->f, l);
}

bool kv_del(kv_store *kv, char *key) {
  if (ht_del(&kv->ht, key)) {
    log l = {.type = DEL, .op = {.del = {.key = key}}};
    aof_append(&kv->f, l);
    return true;
  }
  return false;
}

bool kv_get(kv_store *kv, char *key, buffer *out) {
  return ht_get(&kv->ht, key, out);
}

void kv_dbg(kv_store *kv) { ht_dbg(&kv->ht); }

void kv_close(kv_store *kv) {
  aof_close(&kv->f);
  ht_free(&kv->ht);
}

#endif
