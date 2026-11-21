# Makefile to compile

SRCS=src/engine.c \
     src/fail.c \
     src/main.c \
     src/memory*.c

CFLAGS_COMMON=-std=c11 -fwrapv

# Clang address sanitizer flags
# (see http://clang.llvm.org/docs/AddressSanitizer.html)
CLANG_ASAN_CFLAGS=-fsanitize=address

# Clang undefined behavior sanitizer flags
# (see http://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html)
CLANG_UBSAN_CFLAGS=-fsanitize=undefined-trap -fsanitize-undefined-trap-on-error

# Clang warning flags
CLANG_WARNING_FLAGS=-Wall -Wno-format-nonliteral

# Flags for debugging:
CFLAGS_DEBUG=${CFLAGS_COMMON} -g ${CLANG_ASAN_FLAGS} ${CLANG_UBSAN_FLAGS} ${CLANG_WARNING_FLAGS}

# Flags for maximum performance:
CFLAGS_RELEASE=${CFLAGS_COMMON} -O3 -DNDEBUG

CFLAGS=${CFLAGS_DEBUG}

default:
	@echo "Use the following targets:"
	@echo " - 'make vm' to use your copying GC"
	@echo " - 'make test' to test the VM"
	@echo " - 'make clean' to clean the VM"
	@echo ""
	@echo "Note: the GC_VERSION preprocessor variable controls which"
	@echo "      garbage collector is used in the virtual machine."

bin:
	mkdir -p bin

vm: clean bin/vm

bin/vm: bin ${SRCS}
	${CC} ${CFLAGS} ${LDFLAGS} ${SRCS} -o bin/vm

test: vm
	@((echo 10   | bin/vm ../examples/asm/queens.asm  2>&1) >/dev/null && echo Queens test passed!) || echo Queens test failed!
	@((echo 400 | bin/vm ../examples/asm/bignums.asm 2>&1) >/dev/null && echo Bignums test passed!) || echo Bignums test failed!
	@((echo 120  | bin/vm ../examples/asm/pascal.asm  2>&1) >/dev/null && echo Pascal test passed!) || echo Pascal test failed!
	@((echo 10  | bin/vm ../examples/asm/maze.asm    2>&1) >/dev/null && echo Maze test passed!) || echo Maze test failed!
	@((bin/vm ../examples/asm/test1.asm    2>&1) >/dev/null && echo test1 passed!) || echo test1 failed!
	@((bin/vm ../examples/asm/test2.asm    2>&1) >/dev/null && echo test2 failed!) || echo test2 passed!
	@((bin/vm ../examples/asm/test3.asm    2>&1) >/dev/null && echo test3 passed!) || echo test3 failed!
	@((bin/vm ../examples/asm/test4.asm    2>&1) >/dev/null && echo test4 passed!) || echo test4 failed!
	@((bin/vm ../examples/asm/test5.asm    2>&1) >/dev/null && echo test5 failed!) || echo test5 passed!
	@((bin/vm ../examples/asm/test6.asm    2>&1) >/dev/null && echo test6 passed!) || echo test6 failed!
	@((bin/vm ../examples/asm/test7.asm    2>&1) >/dev/null && echo test7 passed!) || echo test7 failed!
	@((bin/vm ../examples/asm/test8.asm    2>&1) >/dev/null && echo test8 passed!) || echo test8 failed!
	@((bin/vm ../examples/asm/test9.asm    2>&1) >/dev/null && echo test9 passed!) || echo test9 failed!
	@((bin/vm ../examples/asm/test10.asm    2>&1) >/dev/null && echo test10 passed!) || echo test10 failed!

clean:
	rm -rf bin

all: clean vm
