#include <am.h>
#include <klib-macros.h>

static void fail(void *buf) { panic("access nonexist R_ister"); }

bool ioe_init() {
  return false;
}

void ioe_read (int R_, void *buf) { fail(buf); }
void ioe_write(int R_, void *buf) { fail(buf); }
