# Nickel

Nickel is a toy language with a simple interpreter and [LLVM][llvm]-based
[JIT][jit] evaluation mode.

Latest build status: [![CircleCI](https://circleci.com/gh/owst/nickel.svg?style=svg)](https://circleci.com/gh/owst/nickel)

## What's in a name?

Just In Time; In The Nick of Time; Nickel? Sorry! :grimacing:

## Why?

I wanted to try implementing a simple JIT with LLVM... it was fun!

## JIT Example

The following example computes and prints a numeric value:

```ruby
# Saved to input.nkl...

def g(x)
  9 * x
end

def f(x, y)
  (57005 << x) + g(y + 1)
end

puts f(16, 5430)
```

We can evaluate this program with the `nickel` evaluator in interpreter mode -
this will read the program from stdin and print any output to stdout:
```shell
$ ./nickel --interpreter < input.nkl
3735928559
```

The particular value printed will be more familiar with a hex representation:
```shell
$ printf "0x%x\n" $(./nickel --interpreter < input.nkl)
0xdeadbeef
```

To witness the runtime code generation of the JIT evaluator, take a look JIT
evaluator in `jit.c`; we see that we ask LLVM to generate us a function pointer
to an anonymous "top-level" function representing the Nickel program at around
line 284 in `jit.c`:
```
    int (*func)(void) = (int (*)(void))LLVMGetFunctionAddress(engine, "__anon_tl");
    func();
```

Let's run [`lldb`][lldb] and break before the invocation of `func`:

```
$ lldb ./nickel -- --jit
[...]
(lldb) breakpoint set --file jit.c --line 284
[...]
(lldb) process launch -i input.nkl
[...]
* thread #1, queue = 'com.apple.main-thread', stop reason = breakpoint 1.1
    frame #0: 0x00000001000035f9 nickel`jit(p=0x0000000101a08940) at jit.c:284
   281 	    }
   282
   283 	    int (*func)(void) = (int (*)(void))LLVMGetFunctionAddress(engine, "__anon_tl");
-> 284 	    func();
   285
   286 	    LLVMDisposeExecutionEngine(engine);
   287 	}
[...]
(lldb) disassemble -s func
    0x101943030: movabsq $0x101944000, %rdi        ; imm = 0x101944000
    0x10194303a: movabsq $0x7fff67760710, %rcx     ; imm = 0x7FFF67760710
    0x101943044: movl   $0xdeadbeef, %esi         ; imm = 0xDEADBEEF
    0x101943049: xorl   %eax, %eax
    0x10194304b: jmpq   *%rcx
    0x10194304d: addb   %al, (%rax)
```

note how when we disassemble the `func` function, we can see a literal value of
`0xdeadbeef` - our program has been fully optimisated away by LLVM - neat!

## Developing

To build/run tests, simply run `make`. You'll require LLVM, and a few other
tools.

### Tests

`make test` will run some simple end-to-end tests (in `test_runner.sh`) that
verify the correct output for interpreter (default) and JIT modes.

## JIT debugging

To assit debugging the JIT, set `DUMP_BITCODE=true` in the main process'
environment to dump out the generated LLVM module (both before and after
optimisation passes have been applied). The resulting files can be passed to
[`llvm-dis`][llvm-dis] for disassembly. For example:
```shell
$ DUMP_BITCODE=true ./nickel --jit < input.nkl
3735928559
```
We can then run `llvm-dis`:
```llvm
$ llvm-dis < optimised_module.bc
; ModuleID = '<stdin>'
source_filename = "jit_module"

@format = private unnamed_addr constant [5 x i8] c"%ld\0A\00"

declare i64 @printf(...) local_unnamed_addr

; Function Attrs: norecurse nounwind readnone
define i64 @g(i64 %x) local_unnamed_addr #0 {
entry:
  %mul = mul i64 %x, 9
  ret i64 %mul
}

; Function Attrs: norecurse nounwind readnone
define i64 @f(i64 %x, i64 %y) local_unnamed_addr #0 {
entry:
  %shl = shl i64 57005, %x
  %0 = mul i64 %y, 9
  %mul.i = add i64 %shl, 9
  %add1 = add i64 %mul.i, %0
  ret i64 %add1
}

define void @__anon_tl() local_unnamed_addr {
entry:
  %printf = tail call i64 (...) @printf(i8* getelementptr inbounds ([5 x i8], [5 x i8]* @format, i64 0, i64 0), i64 3735928559)
  ret void
}

attributes #0 = { norecurse nounwind readnone }
```
where we can see (if we scroll right!) `3735928559` being passed as a literal
to the `printf` call.

[llvm]: https://llvm.org/
[jit]: https://en.wikipedia.org/wiki/Just-in-time_compilation
[llvm-dis]: https://llvm.org/docs/CommandGuide/llvm-dis.html
[lldb]: https://lldb.llvm.org/
[clang-tidy]: https://clang.llvm.org/extra/clang-tidy/
