// toy_jit for toy_jvm kevinlynx@gmail.com
// 2017.03.05
#include "toy_jit.h"
#include <stdio.h>
#include <stdlib.h>
#include <jit/jit-dump.h>
#include <jit/jit.h>

char* read_file(const char* name, long *size) {
  FILE* fp = fopen(name, "rb");
  fseek(fp, 0 , SEEK_END);
  *size = ftell(fp);
  char* buf = (char*) malloc(*size);
  fseek(fp, 0, SEEK_SET);
  fread(buf, *size, 1, fp);
  fclose(fp);  
  return buf;
}

void dump(const char* name, jit_function_t F) {
  FILE* fp = fopen(name, "wb");
  jit_dump_function(fp, F, "test");
  fclose(fp);
}

extern jit_function_t toy_jit_build(jit_context_t context, const char* bytecodes, int size, int max_locals, 
    int max_labels, int arg_cnt, int ret);

int main(int argc, char** argv) {
  long size = 0;
  char* buf = read_file(argv[1], &size);  
  jit_context_t context = jit_context_create();
  jit_function_t F = toy_jit_build(context, buf, (int) size, 4, 1, 1, 1);
  dump("ir.dump", F);
  free(buf);
  { /* compile */
    jit_context_build_start(context);
    jit_function_compile(F);
    jit_context_build_end(context);
    dump("asm.dump", F);
  }
  { /* test run */
    int n = 4;
    void* args[] = {&n};
    jit_int ret;
    jit_function_apply(F, args, &ret);
    printf("test fac(%d) = %d\n", n, ret); // */
  }
  jit_context_destroy(context);
  return 0;
}
