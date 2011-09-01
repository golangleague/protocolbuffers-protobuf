
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "upb/def.h"
#include "upb/handlers.h"
#include "upb/pb/decoder.h"
#include "upb/pb/glue.h"
#include "upb_test.h"

static upb_symtab *load_test_proto() {
  upb_symtab *s = upb_symtab_new();
  ASSERT(s);
  size_t len;
  char *descriptor = upb_readfile("tests/test.proto.pb", &len);
  if(!descriptor) {
    fprintf(stderr, "Couldn't read input file tests/test.proto.pb\n");
    exit(1);
  }
  upb_status status = UPB_STATUS_INIT;
  upb_read_descriptor(s, descriptor, len, &status);
  ASSERT(upb_ok(&status));
  upb_status_uninit(&status);
  free(descriptor);
  return s;
}

static upb_flow_t upb_test_onvalue(void *closure, upb_value fval, upb_value val) {
  (void)closure;
  (void)fval;
  (void)val;
  return UPB_CONTINUE;
}

static void test_upb_jit() {
  upb_symtab *s = load_test_proto();
  upb_def *def = upb_symtab_lookup(s, "SimplePrimitives");
  ASSERT(def);

  upb_handlers *h = upb_handlers_new();
  upb_handlerset hset = {NULL, NULL, &upb_test_onvalue, NULL, NULL, NULL, NULL};
  upb_handlers_reghandlerset(h, upb_downcast_msgdef(def), &hset);
  upb_decoder d;
  upb_decoder_init(&d, h);
  upb_decoder_uninit(&d);
  upb_symtab_unref(s);
  upb_def_unref(def);
  upb_handlers_unref(h);
}

static void test_upb_symtab() {
  upb_symtab *s = load_test_proto();

  // Test cycle detection by making a cyclic def's main refcount go to zero
  // and then be incremented to one again.
  upb_def *def = upb_symtab_lookup(s, "A");
  ASSERT(def);
  upb_symtab_unref(s);
  upb_msgdef *m = upb_downcast_msgdef(def);
  upb_msg_iter i = upb_msg_begin(m);
  upb_fielddef *f = upb_msg_iter_field(i);
  ASSERT(upb_hassubdef(f));
  upb_def *def2 = f->def;

  i = upb_msg_next(m, i);
  ASSERT(upb_msg_done(i));  // "A" should only have one field.

  ASSERT(upb_downcast_msgdef(def2));
  upb_def_ref(def2);
  upb_def_unref(def);
  upb_def_unref(def2);
}

int main()
{
#define TEST(func) do { \
  int assertions_before = num_assertions; \
  printf("Running " #func "..."); fflush(stdout); \
  func(); \
  printf("ok (%d assertions).\n", num_assertions - assertions_before); \
  } while (0)

  TEST(test_upb_symtab);
  TEST(test_upb_jit);
  printf("All tests passed (%d assertions).\n", num_assertions);
  return 0;
}
