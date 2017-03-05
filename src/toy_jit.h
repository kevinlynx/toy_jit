// toy_jit for toy_jvm kevinlynx@gmail.com

#ifdef __cplusplus
extern "C" {
#endif

typedef struct JITContext JITContext;

JITContext* toyjit_context_init();
void toyjit_context_release(JITContext* context);

int toyjit_build(JITContext* context, const char* bytecodes, int size, int max_locals, 
    int max_labels, int arg_cnt, int ret);

int toyjit_compile(JITContext* context);
int toyjit_apply(JITContext* context, void* args, int* ret);

#ifdef __cplusplus
}
#endif
