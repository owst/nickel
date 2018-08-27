#ifndef JIT_H
#define JIT_H

#include <llvm-c/Core.h>

#include "helpers.h"
#include "syntax.h"

LLVMValueRef jit_expr(LLVMModuleRef mod, LLVMBuilderRef builder, LLVMValueRef func, struct expr *expr);

void jit(struct program *p);

#endif /* JIT_H */
