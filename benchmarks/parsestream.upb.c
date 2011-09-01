
#include "main.c"

#include <stdlib.h>
#include "upb/bytestream.h"
#include "upb/def.h"
#include "upb/pb/decoder.h"
#include "upb/pb/glue.h"

static char *input_str;
static size_t input_len;
static upb_msgdef *def;
static upb_decoder decoder;
static upb_stringsrc stringsrc;

static upb_sflow_t startsubmsg(void *_m, upb_value fval) {
  (void)_m;
  (void)fval;
  return UPB_CONTINUE_WITH(NULL);
}

static upb_flow_t value(void *closure, upb_value fval, upb_value val) {
  (void)closure;
  (void)fval;
  (void)val;
  return UPB_CONTINUE;
}

static bool initialize()
{
  // Initialize upb state, decode descriptor.
  upb_status status = UPB_STATUS_INIT;
  upb_symtab *s = upb_symtab_new();
  upb_read_descriptorfile(s, MESSAGE_DESCRIPTOR_FILE, &status);
  if(!upb_ok(&status)) {
    fprintf(stderr, "Error reading descriptor: %s\n",
            upb_status_getstr(&status));
    return false;
  }

  def = upb_dyncast_msgdef(upb_symtab_lookup(s, MESSAGE_NAME));
  if(!def) {
    fprintf(stderr, "Error finding symbol '%s'.\n", MESSAGE_NAME);
    return false;
  }
  upb_symtab_unref(s);

  // Read the message data itself.
  input_str = upb_readfile(MESSAGE_FILE, &input_len);
  if(input_str == NULL) {
    fprintf(stderr, "Error reading " MESSAGE_FILE "\n");
    return false;
  }

  upb_handlers *handlers = upb_handlers_new();
  if (!JIT) handlers->should_jit = false;
  // Cause all messages to be read, but do nothing when they are.
  upb_handlerset hset = {NULL, NULL, value, startsubmsg, NULL, NULL, NULL};
  upb_handlers_reghandlerset(handlers, def, &hset);
  upb_decoder_init(&decoder, handlers);
  upb_handlers_unref(handlers);
  upb_stringsrc_init(&stringsrc);
  return true;
}

static void cleanup()
{
  free(input_str);
  upb_def_unref(UPB_UPCAST(def));
  upb_decoder_uninit(&decoder);
  upb_stringsrc_uninit(&stringsrc);
}

static size_t run(int i)
{
  (void)i;
  upb_status status = UPB_STATUS_INIT;
  upb_stringsrc_reset(&stringsrc, input_str, input_len);
  upb_decoder_reset(&decoder, upb_stringsrc_bytesrc(&stringsrc),
                    0, UPB_NONDELIMITED, NULL);
  upb_decoder_decode(&decoder, &status);
  if(!upb_ok(&status)) goto err;
  return input_len;

err:
  fprintf(stderr, "Decode error: %s", upb_status_getstr(&status));
  return 0;
}
