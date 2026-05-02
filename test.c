#include "hashtable.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

void test_put_and_get() {
  hash_table ht = ht_init();
  ht_put(&ht, "foo", "bar");
  buffer out;
  assert(ht.len == 1);
  assert(ht_get(&ht, "foo", &out));
  assert(out.len == 3);
  assert(strcmp(out.data, "bar") == 0);
  ht_free(&ht);
}

void test_put_and_update() {
  hash_table ht = ht_init();
  ht_put(&ht, "foo", "bar");
  ht_put(&ht, "foo", "baz");
  buffer out;
  assert(ht.len == 1);
  assert(ht_get(&ht, "foo", &out));
  assert(out.len == 3);
  assert(strcmp(out.data, "baz") == 0);
  ht_free(&ht);
}

void test_put_and_delete() {
  hash_table ht = ht_init();
  ht_put(&ht, "foo", "bar");
  assert(ht_del(&ht, "foo"));
  assert(!ht_get(&ht, "f00", NULL));
  assert(ht.len == 0);
  ht_free(&ht);
}

int main(void) {
  test_put_and_get();
  test_put_and_update();
  test_put_and_delete();
  puts("All test passed!");
}
