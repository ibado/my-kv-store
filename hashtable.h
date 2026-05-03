#ifndef HASHTABLE
#define HASHTABLE

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_SIZE 8
#define MAX_LOAD_FACTOR 0.75

typedef struct buffer {
  char *data;
  size_t len;
} buffer;

buffer buf_new(char *str) {
  assert(str);
  size_t len = strlen(str);
  char *data = malloc(len + 1);
  memcpy(data, str, len + 1);
  assert(data);
  return (buffer){
      .data = data,
      .len = len,
  };
}

void buf_del(buffer *buf) {
  free(buf->data);
  buf->data = NULL;
}

typedef struct ht_entry {
  buffer key;
  buffer value;
  struct ht_entry *colls;
} ht_entry;

typedef struct hash_table {
  ht_entry *slots;
  size_t len;
  size_t cap;
} hash_table;

// api
hash_table ht_init();
void ht_put(hash_table *ht, char *key, char *value);
bool ht_get(hash_table *ht, char *key, buffer *out);
bool ht_del(hash_table *ht, char *key);
void ht_free(hash_table *ht);
void ht_dbg(hash_table *ht);

hash_table ht_with_capacity(size_t cap) {
  hash_table ht = {
      .len = 0,
      .cap = cap,
  };
  ht.slots = (ht_entry *)calloc(cap, sizeof(ht_entry));
  assert(ht.slots);

  return ht;
}

hash_table ht_init() { return ht_with_capacity(DEFAULT_SIZE); }

// FNV-1a
int hash(char *data, size_t len) {
  uint64_t h = 0xcbf29ce484222325; // FNV offset
  for (size_t i = 0; i < len; i++) {
    h ^= (unsigned char)data[i];
    h *= 0x00000100000001B3; // FNV prime
  }

  return h;
}

/* ========= LINKED LIST START ==========*/
// return true if the key is present and put it in *out
bool _ll_get(ht_entry *head, char *key, buffer *out) {
  if (head == NULL)
    return false;
  ht_entry *curr = head;
  while (curr != NULL) {
    if (strcmp(curr->key.data, key) == 0) {
      *out = curr->value;
      return true;
    }
    curr = curr->colls;
  }
  return false;
}

// return true if the key was already there.
bool _ll_upsert(ht_entry **head, buffer key, buffer value) {
  assert(head);
  ht_entry **curr = head;
  while (*curr != NULL) {
    if (strcmp((*curr)->key.data, key.data) == 0) {
      (*curr)->value = value;
      return true;
    }
    curr = &((*curr)->colls);
  }
  *curr = calloc(1, sizeof(ht_entry));
  assert(*curr);
  (*curr)->key = key;
  (*curr)->value = value;
  return false;
}

// return true if the key was present.
bool _ll_del(ht_entry **head, char *key) {
  assert(head);
  if (*head == NULL)
    return false;
  ht_entry **curr = head;
  while (*curr != NULL) {
    if (strcmp((*curr)->key.data, key) == 0) {
      ht_entry *to_remove = *curr;
      *curr = to_remove->colls; // unlink to_remove
      buf_del(&to_remove->key);
      buf_del(&to_remove->value);
      free(to_remove);
      return true;
    }
    curr = &((*curr)->colls);
  }
  return false;
}

// removes the element in the head and returns it.
ht_entry *_ll_pop(ht_entry **head) {
  if (*head == NULL)
    return NULL;
  ht_entry *to_pop = *head;
  *head = (*head)->colls;
  to_pop->colls = NULL;
  return to_pop;
}

void _ll_dbg(ht_entry *head) {
  ht_entry *curr = head;
  while (curr != NULL) {
    printf("->[%s - %s]", curr->key.data, curr->value.data);
    curr = curr->colls;
  }
  puts("");
}
/* ========= LINKED LIST END ==========*/

/* ========= HASH TABLE API IMPLEMENTATION ==========*/
void ht_put(hash_table *ht, char *key, char *value) {
  assert(ht);
  assert(key);
  assert(value);
  double lf = (double)ht->len / ht->cap;
  if (lf >= MAX_LOAD_FACTOR) {
    hash_table new_ht = ht_with_capacity(2 * ht->cap);
    for (size_t i = 0; i < ht->cap; i++) {
      ht_entry slot = ht->slots[i];
      if (slot.key.data) {
        ht_entry *colls = slot.colls;
        while (colls != NULL) {
          ht_entry *removed = _ll_pop(&(colls));
          ht_put(&new_ht, removed->key.data, removed->value.data);
        }
        ht_put(&new_ht, slot.key.data, slot.value.data);
      }
    }
    ht_free(ht);
    *ht = new_ht;
  }
  buffer buf_key = buf_new(key);
  buffer buf_val = buf_new(value);
  int idx = hash(buf_key.data, buf_key.len) % ht->cap;
  if (ht->slots[idx].key.data == NULL) { // slot is free
    ht->slots[idx].key = buf_key;
    ht->slots[idx].value = buf_val;
    ht->len++;
    return;
  }
  if (strcmp(ht->slots[idx].key.data, key) == 0) { // already insertd
    buf_del(&buf_key);
    buf_del(&(ht->slots[idx].value));
    ht->slots[idx].value = buf_val;
    return;
  }
  // upsert it in the colls list
  _ll_upsert(&(ht->slots[idx].colls), buf_key, buf_val);
  ht->len++;
}

bool ht_get(hash_table *ht, char *key, buffer *out) {
  int idx = hash(key, strlen(key)) % ht->cap;
  ht_entry val = ht->slots[idx];
  if (val.key.data) {
    if (strcmp(val.key.data, key) == 0) {
      *out = val.value;
      return true;
    }
    return _ll_get(ht->slots[idx].colls, key, out);
  }
  return false;
}

bool ht_del(hash_table *ht, char *key) {
  int idx = hash(key, strlen(key)) % ht->cap;
  if (ht->slots[idx].key.data == NULL)
    return false;
  if (strcmp(ht->slots[idx].key.data, key) == 0) {
    buf_del(&ht->slots[idx].key);
    buf_del(&ht->slots[idx].value);
    if (ht->slots[idx].colls) {
      ht_entry *head = _ll_pop(&(ht->slots[idx].colls));
      ht->slots[idx].key = head->key;
      ht->slots[idx].value = head->value;
      free(head);
    }
    assert(ht->len > 0);
    ht->len--;
    return true;
  }
  bool deleted = _ll_del(&(ht->slots[idx].colls), key);
  return deleted;
}

void ht_free(hash_table *ht) {
  if (ht->slots) {
    for (size_t i = 0; i < ht->cap; i++) {
      buf_del(&ht->slots[i].key);
      buf_del(&ht->slots[i].value);
      ht_entry *curr = ht->slots[i].colls;
      while (curr != NULL) {
        ht_entry *to_free = _ll_pop(&curr);
        if (to_free != NULL) {
          buf_del(&to_free->key);
          buf_del(&to_free->value);
          free(to_free);
        }
      }
    }
    free(ht->slots);
    ht->slots = NULL;
  }
}

void ht_dbg(hash_table *ht) {
  puts("=============================");
  printf("Load factor: %f\n", (float)ht->len / ht->cap);
  for (size_t i = 0; i < ht->cap; i++) {
    if (ht->slots[i].key.data) {
      printf("(%s => %s)", ht->slots[i].key.data, ht->slots[i].value.data);
      ht_entry *aux = ht->slots[i].colls;
      while (aux != NULL) {
        printf(" -> (%s => %s)", aux->key.data, aux->value.data);
        aux = aux->colls;
      }
      puts("");
    } else {
      puts("empty slot");
    }
  }
  puts("=============================");
}

#endif
