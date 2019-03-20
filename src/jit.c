#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>
#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/Transforms/PassManagerBuilder.h>

#include "jit.h"

void jit_declare_definition_entry(LLVMModuleRef mod, struct definition_entry *definition_entry) {
    struct func *func = definition_entry->value;
    unsigned long param_count = linked_list_count((struct link *)func->params);
    LLVMTypeRef param_types[param_count];
    for(unsigned long i = 0; i < param_count; i++) {
        param_types[i] = LLVMInt64Type();
    }

    LLVMTypeRef ret_type = LLVMFunctionType(LLVMInt64Type(), param_types, param_count, false);
    LLVMValueRef f = LLVMAddFunction(mod, func->ident->name, ret_type);

    LLVMValueRef args[param_count];
    LLVMGetParams(f, args);

    struct param_entry *param_entry = func->params;

    for(unsigned long i = 0; i < param_count; i++) {
        char *name = param_entry->value->name;
        LLVMSetValueName2(args[i], name, strlen(name)); /* Flawfinder: ignore */
        param_entry = next_entry(param_entry, struct param_entry);
    }
}

void jit_define_definition_entry(LLVMModuleRef mod, struct definition_entry *definition_entry) {
    struct func *func = definition_entry->value;
    LLVMValueRef f = LLVMGetNamedFunction(mod, func->ident->name);

    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(f, "entry");

    LLVMBuilderRef builder = LLVMCreateBuilder();
    LLVMPositionBuilderAtEnd(builder, entry);

    LLVMBuildRet(builder, jit_expr(mod, builder, f, func->body));
}

LLVMValueRef jit_cmp(LLVMBuilderRef builder, LLVMValueRef left, LLVMValueRef right, LLVMIntPredicate op) {
    LLVMValueRef cmp = LLVMBuildICmp(builder, op, left, right, "cmp");
    LLVMValueRef cmp_32 = LLVMBuildIntCast(builder, cmp, LLVMInt64Type(), "cmp_cast");

    // For compatability with the interpreter, return 0 for false, 1 for true.
    LLVMValueRef one = LLVMConstInt(LLVMInt64Type(), 1, false);
    return LLVMBuildAnd(builder, cmp_32, one, "&&");
}


LLVMValueRef jit_le(LLVMBuilderRef builder, LLVMValueRef left, LLVMValueRef right) {
    return jit_cmp(builder, left, right, LLVMIntSLE);
}

LLVMValueRef jit_binop(LLVMModuleRef mod, LLVMBuilderRef builder, LLVMValueRef func, struct binop b) {
    LLVMValueRef left = jit_expr(mod, builder, func, b.l);
    LLVMValueRef right = jit_expr(mod, builder, func, b.r);

    switch (b.type) {
        case EQ:
            return jit_cmp(builder, left, right, LLVMIntEQ);
        case PLUS:
            return LLVMBuildAdd(builder, left, right, "add");
        case MINUS:
            return LLVMBuildSub(builder, left, right, "sub");
        case LSHIFT:
            return LLVMBuildShl(builder, left, right, "shl");
        case MULT:
            return LLVMBuildMul(builder, left, right, "mul");
        case LE:
            return jit_le(builder, left, right);
    }
}

LLVMValueRef jit_conditional(LLVMModuleRef mod, LLVMBuilderRef builder, LLVMValueRef func, struct conditional c) {
    LLVMValueRef cond = jit_expr(mod, builder, func, c.cond);

    LLVMBasicBlockRef then_block = LLVMAppendBasicBlock(func, "then");
    LLVMBasicBlockRef else_block = LLVMAppendBasicBlock(func, "else");
    LLVMBasicBlockRef merge_block = LLVMAppendBasicBlock(func, "merge");

    LLVMValueRef zero = LLVMConstInt(LLVMInt64Type(), 0, false);
    LLVMValueRef cmp = LLVMBuildICmp(builder, LLVMIntNE, cond, zero, "nz");
    LLVMBuildCondBr(builder, cmp, then_block, else_block);

    LLVMPositionBuilderAtEnd(builder, then_block);
    LLVMValueRef then_expr = jit_expr(mod, builder, func, c.on_true);
    LLVMBuildBr(builder, merge_block);
    LLVMBasicBlockRef after_then_block = LLVMGetInsertBlock(builder);

    LLVMPositionBuilderAtEnd(builder, else_block);
    LLVMValueRef else_expr = jit_expr(mod, builder, func, c.on_false);
    LLVMBuildBr(builder, merge_block);
    LLVMBasicBlockRef after_else_block = LLVMGetInsertBlock(builder);

    LLVMPositionBuilderAtEnd(builder, merge_block);
    LLVMValueRef phi_node = LLVMBuildPhi(builder, LLVMInt64Type(), "conditional");
    LLVMAddIncoming(phi_node, &then_expr, &after_then_block, 1);
    LLVMAddIncoming(phi_node, &else_expr, &after_else_block, 1);

    return phi_node;
}

LLVMValueRef jit_puts(LLVMModuleRef mod, LLVMBuilderRef builder, LLVMValueRef func, struct puts p) {
    LLVMValueRef result = jit_expr(mod, builder, func, p.body);
    LLVMValueRef format = LLVMBuildGlobalStringPtr(builder, "%ld\n", "format");

    LLVMValueRef printf_function = LLVMGetNamedFunction(mod, "printf");
    LLVMValueRef printf_args[] = { format, result };

    return LLVMBuildCall(builder, printf_function, printf_args, 2, "printf");
}

LLVMValueRef jit_var(LLVMValueRef func, struct ident *search_ident) {
    size_t name_len;
    unsigned param_count = LLVMCountParams(func);
    LLVMValueRef args[param_count];

    LLVMGetParams(func, args);

    for(unsigned long i = 0; i < param_count; i++) {
        LLVMValueRef arg = args[i];
        const char *name = LLVMGetValueName2(arg, &name_len);

        if (strcmp(name, search_ident->name) == 0) {
            return arg;
        }
    }

    const char *func_name = LLVMGetValueName2(func, &name_len);
    die("Could not find var %s in stack frame of %s!", search_ident->name, func_name);
}

LLVMValueRef jit_call(LLVMModuleRef mod, LLVMBuilderRef builder, LLVMValueRef func, struct call call) {
    LLVMValueRef f = LLVMGetNamedFunction(mod, call.callee->name);

    if (!f) {
        die("Could not find named function %s", call.callee->name);
    }

    unsigned long arg_count = linked_list_count((struct link *)call.args);
    unsigned param_count = LLVMCountParams(f);

    if (arg_count != param_count) {
        die("Urk, bad arg count");
    }

    LLVMValueRef args[arg_count];
    struct arg_entry *arg_entry = call.args;

    for (unsigned int i = 0; i < arg_count; i++) {
        args[i] = jit_expr(mod, builder, func, arg_entry->value);
        arg_entry = next_entry(arg_entry, struct arg_entry);
    }

    return LLVMBuildCall(builder, f, args, arg_count, call.callee->name);
}

LLVMValueRef jit_expr(LLVMModuleRef mod, LLVMBuilderRef builder, LLVMValueRef func, struct expr *expr) {
    switch (expr->type) {
        case LITERAL:
            return LLVMConstInt(LLVMInt64Type(), expr->literal, false);
        case VAR:
            return jit_var(func, expr->var);
        case BINOP:
            return jit_binop(mod, builder, func, expr->binop);
        case CONDITIONAL:
            return jit_conditional(mod, builder, func, expr->conditional);
        case PUTS:
            return jit_puts(mod, builder, func, expr->puts);
        case CALL:
            return jit_call(mod, builder, func, expr->call);
    }
}

void declare_printf(LLVMModuleRef mod) {
    LLVMTypeRef printf_args_ty_list[] = { LLVMPointerType(LLVMInt8Type(), 0) };
    LLVMTypeRef printf_ty =
        LLVMFunctionType(LLVMInt64Type(), printf_args_ty_list, 0, true);

    LLVMAddFunction(mod, "printf", printf_ty);
}

void declare_functions(LLVMModuleRef mod, struct definition_entry *definition_entry) {
    while (definition_entry != NULL) {
        jit_declare_definition_entry(mod, definition_entry);

        definition_entry = next_entry(definition_entry, struct definition_entry);
    }
}

void define_functions(LLVMModuleRef mod, struct definition_entry *definition_entry) {
    while (definition_entry != NULL) {
        jit_define_definition_entry(mod, definition_entry);

        definition_entry = next_entry(definition_entry, struct definition_entry);
    }
}

void jit_expr_into_anonymous_function(LLVMModuleRef mod, struct expr *expr) {
    LLVMTypeRef ret_type = LLVMFunctionType(LLVMVoidType(), NULL, 0, false);
    LLVMValueRef anon_tl = LLVMAddFunction(mod, "__anon_tl", ret_type);
    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(anon_tl, "entry");

    LLVMBuilderRef builder = LLVMCreateBuilder();
    LLVMPositionBuilderAtEnd(builder, entry);

    jit_expr(mod, builder, anon_tl, expr);

    LLVMBuildRetVoid(builder);
    LLVMDisposeBuilder(builder);
}

void apply_optimisation_passes(LLVMModuleRef mod) {
    LLVMPassManagerBuilderRef pass_manager_builder = LLVMPassManagerBuilderCreate();
    LLVMPassManagerBuilderUseInlinerWithThreshold(pass_manager_builder, 225);
    LLVMPassManagerBuilderSetOptLevel(pass_manager_builder, 3);
    LLVMPassManagerRef pass_manager = LLVMCreatePassManager();

    LLVMPassManagerBuilderPopulateModulePassManager(pass_manager_builder, pass_manager);
    LLVMRunPassManager(pass_manager, mod);

    LLVMDisposePassManager(pass_manager);
}

void dump_bitcode(LLVMModuleRef mod, const char *filename) {
    const char *jit_debug = getenv("DUMP_BITCODE"); /* Flawfinder: ignore */

    if (!jit_debug || strcmp(jit_debug, "true") != 0) {
        return;
    }

    if (LLVMWriteBitcodeToFile(mod, filename) != 0) {
        fprintf(stderr, "error writing bitcode to file, skipping\n");
    }
}

void jit(struct program *p) {
    LLVMModuleRef mod = LLVMModuleCreateWithName("jit_module");

    declare_printf(mod);
    declare_functions(mod, p->funcs);
    define_functions(mod, p->funcs);

    jit_expr_into_anonymous_function(mod, p->expr);

    // Write out unoptimised bitcode to file
    dump_bitcode(mod, "unoptimised_module.bc");

    char *error = NULL;
    LLVMVerifyModule(mod, LLVMAbortProcessAction, &error);
    LLVMDisposeMessage(error);

    apply_optimisation_passes(mod);

    // Write out optimised bitcode to file
    dump_bitcode(mod, "optimised_module.bc");

    LLVMExecutionEngineRef engine;
    error = NULL;
    LLVMLinkInMCJIT();
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();
    if (LLVMCreateExecutionEngineForModule(&engine, mod, &error) != 0) {
        fprintf(stderr, "failed to create execution engine\n");
        exit(1);
    }
    if (error) {
        fprintf(stderr, "error: %s\n", error);
        LLVMDisposeMessage(error);
        exit(1);
    }

    int (*func)(void) = (int (*)(void))LLVMGetFunctionAddress(engine, "__anon_tl");
    func();

    LLVMDisposeExecutionEngine(engine);
}
