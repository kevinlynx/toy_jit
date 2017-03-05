// export to JNI
#include <jni.h>
#include "toy_jit.h"
#include <stdio.h>
#include <stdlib.h>

#define TRACE(s, args...) printf(s, ##args)

static jlong compile(JNIEnv* env, jclass clazz, jbyteArray ptr, int max_locals, int max_labels,
    int arg_cnt, int ret_type) {
  jbyte* bytes = (*env)->GetByteArrayElements(env, ptr, NULL);
  jsize len = (*env)->GetArrayLength(env, ptr);  
  TRACE("compile called with <%d> bytes: %d,%d,%d,%d\n", len, max_locals, max_labels, arg_cnt, ret_type);
  JITContext* jit = toyjit_context_init();
  toyjit_build(jit, (char*) bytes, len, max_locals, max_labels, arg_cnt, ret_type); 
  toyjit_compile(jit);
  (*env)->ReleaseByteArrayElements(env, ptr, bytes, 0);
  return (jlong) jit;
}

static jint invoke(JNIEnv* env, jclass clazz, jlong jit_ptr, jintArray ptr) {
  JITContext* jit = (JITContext*) jit_ptr;
  jint* args_data = (*env)->GetIntArrayElements(env, ptr, NULL);
  jsize len = (*env)->GetArrayLength(env, ptr);  
  void **args = (void**) malloc(sizeof(args[0]) * len);
  for (int i = 0; i < len; ++i) {
    args[i] = &args_data[i];
  }
  int ret = 0;
  toyjit_apply(jit, args, &ret);
  TRACE("invoke 0x%p by %d args with ret %d\n", jit, len, ret);
  free(args);
  return ret;
}

static JNINativeMethod method_table[] = {
  {"compile", "([BIIII)J", (void*) compile},
  {"invoke", "(J[I)I", (void*) invoke},
};

static int method_table_size = sizeof(method_table) / sizeof(method_table[0]);

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
  TRACE("JNI_OnLoad\n");
  JNIEnv* env;
  if ((*vm)->GetEnv(vm, (void **) &env, JNI_VERSION_1_6) != JNI_OK) {
    return JNI_ERR;
  }
  jclass clazz = (*env)->FindClass(env, "com/codemacro/jvm/jit/ToyJIT");
  if (clazz) {
    jint ret = (*env)->RegisterNatives(env, clazz, method_table, method_table_size);
    (*env)->DeleteLocalRef(env, clazz);
    TRACE("RegisterNatives ret: %d\n", ret);
    return ret == 0 ? JNI_VERSION_1_6 : JNI_ERR;
  }
  return JNI_ERR;
} 
