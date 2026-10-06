#ifndef PLATFORM_STORAGE_H
#define PLATFORM_STORAGE_H
#include "core/ttt_persist.h"
/* Pebble native persistent storage behind the platform-independent TttStorage interface. */
void platform_storage_init(TttStorage *st);
#endif
