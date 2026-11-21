/* l3runtime.h
 * L3 block runtime for the MiniScala → C → WASM backend.
 * Include this at the top of every emitted .c file.
 * Compile with: emcc -O2 program.c -o program.wasm
 */
#ifndef L3RUNTIME_H
#define L3RUNTIME_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

/* ── Uniform value type ─────────────────────────────────────────── */
typedef int64_t L3Val;

/* ── Heap block ─────────────────────────────────────────────────── */
/*
 * Every heap-allocated L3 object has this layout in memory:
 *
 *   [ tag (32-bit) | size (32-bit) | fields[0] | ... | fields[N-1] ]
 *
 * 'tag'  mirrors the integer passed to BALO in the CPS IR.
 * 'size' is the number of L3Val fields (i.e. what BSIZ returns).
 * 'fields' is a C99 flexible array member — zero overhead.
 *
 * An L3Val holding a block pointer is just a cast intptr_t.
 * Integers are stored directly; the CPS IR never mixes the two
 * in a way that requires tagging at this level.
 */
typedef struct L3Block {
    uint32_t tag;
    uint32_t size;
    L3Val    fields[];   /* flexible array member — C99/C11 */
} L3Block;

/* ── Block allocation ───────────────────────────────────────────── */
/*
 * gc_alloc corresponds directly to BALO(a, size_reg, tag_literal).
 * Returns an L3Val so the emitter can assign it straight to a register
 * variable without a cast at every call site.
 *
 * Allocation is a thin malloc wrapper for now. Swap in Boehm GC or
 * a bump allocator here without changing any emitted code.
 */
static inline L3Val gc_alloc(uint32_t tag, L3Val size) {
    L3Block *blk = (L3Block*) malloc(sizeof(L3Block) + (size_t)size * sizeof(L3Val));
    if (!blk) { fputs("L3: out of memory\n", stderr); exit(1); }
    blk->tag  = tag;
    blk->size = (uint32_t)size;
    return (L3Val)(intptr_t)blk;
}

/* ── Block accessors ────────────────────────────────────────────── */
/* BTAG(a, b)  →  a = l3_tag(b)  */
static inline L3Val l3_tag(L3Val v) {
    return (L3Val)((L3Block *)(intptr_t)v)->tag;
}

/* BSIZ(a, b)  →  a = l3_size(b) */
static inline L3Val l3_size(L3Val v) {
    return (L3Val)((L3Block *)(intptr_t)v)->size;
}

/* BGET(a, b, c)  →  a = l3_get(b, c) */
static inline L3Val l3_get(L3Val blk, L3Val idx) {
    return ((L3Block *)(intptr_t)blk)->fields[idx];
}

/* BSET(_, a, b, c)  →  l3_set(a, b, c) */
static inline void l3_set(L3Val blk, L3Val idx, L3Val val) {
    ((L3Block *)(intptr_t)blk)->fields[idx] = val;
}

/* ── I/O primitives ─────────────────────────────────────────────── */
/* BREA(a)  →  a = l3_brea() */
static inline L3Val l3_brea(void) {
    int c = getchar();
    return (c == EOF) ? -1 : (L3Val)c;
}

/* BWRI(a)  →  l3_bwri(a) */
static inline void l3_bwri(L3Val v) {
    putchar((int)v);
}

#endif /* L3RUNTIME_H */
