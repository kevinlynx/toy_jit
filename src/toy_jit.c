// toy_jit for toy_jvm kevinlynx@gmail.com
// 2017.03.05
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <jit/jit.h>
#include "toy_jit.h"

typedef enum Opcode {
  op_mov = 0x10,
  op_lod = 0x11,
  op_mul = 0x12,
  op_div = 0x13,
  op_add = 0x14,
  op_sub = 0x15,
  op_jmp_gt = 0x16,
  op_jmp_ge = 0x17,
  op_jmp_eq = 0x18,
  op_jmp_ne = 0x19,
  op_jmp_lt = 0x1a,
  op_jmp_le = 0x1b,
  op_ret = 0x1c,
  op_label = 0x1d,
} Opcode;

typedef struct Instruction {
  int code;
  int op1, op2;
} Instruction;

typedef struct BuildContext {
  jit_function_t F;
  jit_value_t* vars;
  int var_cnt;
  jit_label_t* labels;
  int label_cnt;
} BuildContext;

typedef void (*Builder) (BuildContext* context, const Instruction* inst);

static void build_mov(BuildContext* context, const Instruction* inst) {
  jit_value_t c = jit_value_create_nint_constant(context->F, jit_type_int, inst->op1); 
  jit_insn_store(context->F, context->vars[inst->op2], c);
}

static void build_lod(BuildContext* context, const Instruction* inst) {
  jit_insn_store(context->F, context->vars[inst->op2], context->vars[inst->op1]);
}

static void build_mul(BuildContext* context, const Instruction* inst) {
  jit_value_t tmp = jit_insn_mul(context->F, context->vars[inst->op1], context->vars[inst->op2]);
  jit_insn_store(context->F, context->vars[inst->op1], tmp);
}

static void build_sub(BuildContext* context, const Instruction* inst) {
  jit_value_t tmp = jit_insn_sub(context->F, context->vars[inst->op1], context->vars[inst->op2]);
  jit_insn_store(context->F, context->vars[inst->op1], tmp);
}

static void build_label(BuildContext* context, const Instruction* inst) {
  jit_insn_label(context->F, &context->labels[inst->op1]);
}

static void build_jmp_gt(BuildContext* context, const Instruction* inst) {
  jit_value_t const0 = jit_value_create_nint_constant(context->F, jit_type_int, 0);
  jit_value_t cmp_v_0 = jit_insn_gt(context->F, context->vars[inst->op1], const0);
  jit_insn_branch_if(context->F, cmp_v_0, &context->labels[inst->op2]);
}

static void build_jmp_le(BuildContext* context, const Instruction* inst) {
  jit_value_t const0 = jit_value_create_nint_constant(context->F, jit_type_int, 0);
  jit_value_t cmp_v_0 = jit_insn_le(context->F, context->vars[inst->op1], const0);
  jit_insn_branch_if(context->F, cmp_v_0, &context->labels[inst->op2]);
}

static void build_ret(BuildContext* context, const Instruction* inst) {
  jit_insn_return(context->F, context->vars[inst->op1]);
}

typedef struct BuilderFactory {
  int opcode;
  Builder builder;
} BuilderFactory;

static BuilderFactory builders[] = {
  {op_mov, build_mov},
  {op_lod, build_lod},
  {op_mul, build_mul},
  {op_sub, build_sub},
  {op_label, build_label},
  {op_jmp_gt, build_jmp_gt},
  {op_jmp_le, build_jmp_le},
  {op_ret, build_ret},
};

Builder find_builder(int opcode) {
  for (size_t i = 0; i < sizeof(builders) / sizeof(builders[0]); ++i) {
    if (builders[i].opcode == opcode) return builders[i].builder;
  }
  return NULL;
}

void init_build_context(BuildContext* context, jit_function_t F, int max_locals, int max_labels, 
    int arg_cnt) {
  context->F = F;
  context->var_cnt = max_locals;
  context->vars = (jit_value_t*) malloc(sizeof(context->vars[0]) * max_locals);
  for (int i = 0; i < arg_cnt; ++i) {
    context->vars[i] = jit_value_get_param(F, i);
  }
  for (int i = arg_cnt; i < max_locals; ++i) {
    context->vars[i] = jit_value_create(F, jit_type_int);
  }
  context->label_cnt = max_labels;
  context->labels = (jit_label_t*) malloc(sizeof(context->labels[0]) * max_labels);
  for (int i = 0; i < max_labels; ++i) {
    context->labels[i] = jit_label_undefined;  
  }
}

void release_build_context(BuildContext* context) {
  free(context->vars);
  free(context->labels);
}

#define READ_BYTE(p) *((unsigned char*)(p))
#define READ_INT(p) (int)(((p)[0] << 24) | ((p)[1] << 16) | ((p)[2] << 8) | (p)[3])

#define PANIC(s) fprintf(stderr, "panic exit: " s); 

jit_function_t toy_jit_build(jit_context_t context, const char* bytecodes, int size, int max_locals, 
    int max_labels, int arg_cnt, int ret) {
  jit_context_build_start(context);
  jit_type_t ret_type = ret == 0 ? jit_type_void : jit_type_int;
  jit_type_t *args = arg_cnt == 0 ? NULL : (jit_type_t*) malloc(sizeof(args[0]) * arg_cnt);
  for (int i = 0; i < arg_cnt; ++i) {
    args[i] = jit_type_int;
  }
  jit_type_t signature = jit_type_create_signature(jit_abi_cdecl, ret_type, args, arg_cnt, 0);
  jit_function_t F = jit_function_create(context, signature);
  jit_type_free(signature);
  free(args);
  int pos = 0;
  BuildContext build_context;
  init_build_context(&build_context, F, max_locals, max_labels, arg_cnt);

  while (pos < size) {
    int code = READ_BYTE(bytecodes + pos);
    int op1 = READ_INT((unsigned char*)bytecodes + pos + 1);
    int op2 = READ_INT((unsigned char*)bytecodes + pos + 5);
    pos += 9;
    Builder builder = find_builder(code);    
    if (builder == NULL) {
      PANIC("not found opcode builder");
      continue;
    }
    Instruction inst = {code, op1, op2};
    builder(&build_context, &inst);
  }
  jit_context_build_end(context);
  release_build_context(&build_context);
  return F;
}

// API
typedef struct JITContext {
  jit_context_t context;
  jit_function_t F;
} JITContext;

JITContext* toyjit_context_init() {
  JITContext* context = (JITContext*) malloc(sizeof(*context));
  context->context = jit_context_create();
  return context;
}

void toyjit_context_release(JITContext* context) {
  jit_context_destroy(context->context);
  free(context);
}

int toyjit_build(JITContext* context, const char* bytecodes, int size, int max_locals, 
    int max_labels, int arg_cnt, int ret) {
  context->F = toy_jit_build(context->context, bytecodes, (int) size, max_locals, max_labels, arg_cnt, ret);
  return 0;
}

int toyjit_compile(JITContext* context) {
  jit_context_build_start(context->context);
  jit_function_compile(context->F);
  jit_context_build_end(context->context);
  return 0;
}

int toyjit_apply(JITContext* context, void* args, int* iret) {
  jit_int ret;
  jit_function_apply(context->F, args, &ret);
  *iret = ret;
  return 0;
}

