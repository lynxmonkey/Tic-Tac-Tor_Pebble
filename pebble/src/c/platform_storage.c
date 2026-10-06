#include <pebble.h>
#include "platform_storage.h"

static int  ps_read(void *c, uint32_t key, void *buf, size_t len) { (void)c; return persist_read_data(key, buf, len); }
static bool ps_write(void *c, uint32_t key, const void *buf, size_t len) {
    (void)c; int n = persist_write_data(key, buf, len); return n >= 0 && (size_t)n == len;
}
static void ps_remove(void *c, uint32_t key) { (void)c; if (persist_exists(key)) persist_delete(key); }
static bool ps_exists(void *c, uint32_t key) { (void)c; return persist_exists(key); }

void platform_storage_init(TttStorage *st) {
    st->ctx = NULL; st->read = ps_read; st->write = ps_write; st->remove = ps_remove; st->exists = ps_exists;
}
