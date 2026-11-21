#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "fail.h"
#include "engine.h"

#if GC_VERSION == GC_MARK_N_SWEEP

static void* memory_start = NULL;
static void* memory_end = NULL;

static uvalue_t* bitmap_start = NULL;

static value_t* heap_start = NULL;
static value_t* heap_end = NULL;
static value_t heap_start_v = 0;
static value_t heap_end_v = 0;
static value_t* heap_first_block = NULL;

#define FREE_LISTS_COUNT 32
static value_t* free_list_heads[FREE_LISTS_COUNT];

#define MIN_BLOCK_SIZE 1
#define HEADER_SIZE 1
// #define BLK_SIZ sizeof(value_t)

// Header management

static value_t header_pack(tag_t tag, value_t size) {
  return (size << 8) | (value_t)tag;
}

static tag_t header_unpack_tag(value_t header) {
  return (tag_t)(header & 0xFF);
}

static value_t header_unpack_size(value_t header) {
  return header >> 8;
}

// Bitmap management

static int bitmap_is_bit_set(value_t* ptr) {
  assert(heap_start <= ptr && ptr < heap_end);
  long index = ptr - heap_start;
  long word_index = index / (long)VALUE_BITS;
  long bit_index = index % (long)VALUE_BITS;
  return (bitmap_start[word_index] & ((uvalue_t)1 << bit_index)) != 0;
}

static void bitmap_set_bit(value_t* ptr) {
  assert(heap_start <= ptr && ptr < heap_end);
  long index = ptr - heap_start;
  long word_index = index / (long)VALUE_BITS;
  long bit_index = index % (long)VALUE_BITS;
  bitmap_start[word_index] |= (uvalue_t)1 << bit_index;
}

static void bitmap_clear_bit(value_t* ptr) {
  assert(heap_start <= ptr && ptr < heap_end);
  long index = ptr - heap_start;
  long word_index = index / (long)VALUE_BITS;
  long bit_index = index % (long)VALUE_BITS;
  bitmap_start[word_index] &= ~((uvalue_t)1 << bit_index);
}

// Virtual <-> physical address translation

static void* addr_v_to_p(value_t v_addr) {
  return (char*)memory_start + v_addr;
}

static value_t addr_p_to_v(void* p_addr) {
  return (value_t)((char*)p_addr - (char*)memory_start);
}

// Free lists management

static value_t real_size(value_t size) {
  assert(0 <= size);
  return size < MIN_BLOCK_SIZE ? MIN_BLOCK_SIZE : size;
}

static unsigned int free_list_index(value_t size) {
  assert(0 <= size);
  return size >= FREE_LISTS_COUNT ? FREE_LISTS_COUNT - 1 : (unsigned int)size;
}

char* memory_get_identity() {
  return "mark & sweep garbage collector";
}

void memory_setup(size_t total_byte_size) {
  memory_start = malloc(total_byte_size);
  if (memory_start == NULL)
    fail("cannot allocate %zd bytes of memory", total_byte_size);
  memory_end = (char*)memory_start + total_byte_size;
}

void memory_cleanup() {
  assert(memory_start != NULL);
  free(memory_start);

  memory_start = memory_end = NULL;
  bitmap_start = NULL;
  heap_start = heap_end = NULL;
  heap_start_v = heap_end_v = 0;
  for (int l = 0; l < FREE_LISTS_COUNT; ++l)
    free_list_heads[l] = NULL;
}

void* memory_get_start() {
  return memory_start;
}

void* memory_get_end() {
  return memory_end;
}

void memory_set_heap_start(void* ptr) {
  assert(memory_start <= ptr && ptr < memory_end);

  const size_t bh_size =
    (size_t)((char*)memory_end - (char*)ptr) / sizeof(value_t);

  const size_t bitmap_size = (bh_size - 1) / (VALUE_BITS + 1) + 1;
  const size_t heap_size = bh_size - bitmap_size;

  bitmap_start = ptr;
  memset(bitmap_start, 0, bitmap_size * sizeof(value_t));

  heap_start = (value_t*)bitmap_start + bitmap_size;
  heap_end = heap_start + heap_size;
  assert(heap_end == memory_end);

  heap_start_v = addr_p_to_v(heap_start);
  heap_end_v = addr_p_to_v(heap_end);

  heap_first_block = heap_start + HEADER_SIZE;
  const value_t initial_block_size = (value_t)(heap_end - heap_first_block);
  heap_first_block[-1] = header_pack(tag_None, initial_block_size);
  heap_first_block[0] = 0;

  for (int l = 0; l < FREE_LISTS_COUNT - 1; ++l)
    free_list_heads[l] = memory_start;
  free_list_heads[FREE_LISTS_COUNT - 1] = heap_first_block;

  // printf("TOTAL FREE BLOCKS: %d\n", initial_block_size);
}

static value_t* allocate(tag_t tag, value_t size) {
  assert(0 <= size);
  // TODO
  size = real_size(size);
  value_t *walker = NULL;
  value_t *blk_ptr = NULL;
  value_t blk_val = 0;
  value_t blk_siz = heap_end_v - heap_start_v;
  // Get free list index
  unsigned int findx = free_list_index(size);
  // Check validity and get chunk
  if (findx == (FREE_LISTS_COUNT-1) || free_list_heads[findx] == memory_start) {
    // Breaking required
    while (findx != (FREE_LISTS_COUNT-1) && free_list_heads[findx] == memory_start)
      findx++;
    // Traverse list
    if (findx == (FREE_LISTS_COUNT-1)) {
      // Find best fit block (if exist)
      walker = free_list_heads[findx];
      while (walker != memory_start) {
        blk_val = header_unpack_size(walker[-1]);
        if (blk_val >= size && blk_val < blk_siz) {
          blk_siz = blk_val;
          blk_ptr = walker;
        }
        walker = addr_v_to_p(walker[0]);
      }
      // Check any available block
      if (blk_ptr) {
        // Remove from free list (check if is head)
        if (free_list_heads[findx] == blk_ptr) free_list_heads[findx] = addr_v_to_p(blk_ptr[0]);
        else {
          walker = free_list_heads[findx];
          blk_val = addr_p_to_v(blk_ptr);
          // Walk list to find prev and remove blk_ptr
          while (walker[0] != blk_val) walker = addr_v_to_p(walker[0]);
          walker[0] = blk_ptr[0];
        }
        // Check if break
        if (blk_siz >= (size + HEADER_SIZE + MIN_BLOCK_SIZE)) {
          // Replace content with new
          blk_ptr[0] = 0;
          blk_val = header_unpack_tag(blk_ptr[-1]);
          blk_ptr[-1] = header_pack(tag, size);
          bitmap_set_bit(blk_ptr);
          // Get remaining free mem after new header
          blk_siz = blk_siz - size - HEADER_SIZE;
          walker = blk_ptr + size + HEADER_SIZE;
          walker[-1] = header_pack(blk_val, blk_siz);
          // Insert into free list
          findx = free_list_index(blk_siz);
          walker[0] = addr_p_to_v(free_list_heads[findx]);
          free_list_heads[findx] = walker;
        }
        else {
          // Imperfect fit
          blk_ptr[0] = 0;
          blk_ptr[-1] = header_pack(tag, blk_siz);
          bitmap_set_bit(blk_ptr);
        }
      }
    }
    else {
      // Take first available
      blk_ptr = free_list_heads[findx];
      free_list_heads[findx] = addr_v_to_p(blk_ptr[0]);
      blk_siz = header_unpack_size(blk_ptr[-1]);
      // Check break or over
      if (blk_siz >= (size + HEADER_SIZE + MIN_BLOCK_SIZE)) {
        // Replace content with new
        blk_ptr[0] = 0;
        blk_val = header_unpack_tag(blk_ptr[-1]);
        // Get remaining free mem after new header
        blk_siz = blk_siz - size - HEADER_SIZE;
        blk_ptr[-1] = header_pack(tag, size);
        bitmap_set_bit(blk_ptr);
        walker = blk_ptr + size + HEADER_SIZE;
        walker[-1] = header_pack(blk_val, blk_siz);
        // Insert into free list
        findx = free_list_index(blk_siz);
        walker[0] = addr_p_to_v(free_list_heads[findx]);
        free_list_heads[findx] = walker;
      }
      else {
        // Imperfect fit
        blk_ptr[0] = 0;
        blk_ptr[-1] = header_pack(tag, blk_siz);
        bitmap_set_bit(blk_ptr);
      }
    }
  }
  else {
    // Perfect fit
    blk_ptr = free_list_heads[findx];
    free_list_heads[findx] = addr_v_to_p(blk_ptr[0]);
    blk_ptr[0] = 0;
    blk_ptr[-1] = header_pack(tag, size);
    bitmap_set_bit(blk_ptr);
  }
  return blk_ptr;

  // Single list
  // value_t rsize = real_size(size);
  // value_t *walker = free_list_heads[FREE_LISTS_COUNT-1];
  // value_t *blk_ptr = NULL;
  // value_t blk_val = 0;
  // value_t blk_siz = heap_end_v - heap_start_v;
  // while (walker != memory_start) {
  //   blk_val = header_unpack_size(walker[-1]);
  //   if (blk_val >= rsize && blk_val < blk_siz) {
  //     blk_siz = blk_val;
  //     blk_ptr = walker;
  //   }
  //   walker = addr_v_to_p(walker[0]);
  // }
  // if (blk_ptr) {
  //   // Remove from free list (check if is head)
  //   if (free_list_heads[FREE_LISTS_COUNT-1] == blk_ptr) free_list_heads[FREE_LISTS_COUNT-1] = addr_v_to_p(blk_ptr[0]);
  //   else {
  //     walker = free_list_heads[FREE_LISTS_COUNT-1];
  //     blk_val = addr_p_to_v(blk_ptr);
  //     // Walk list to find prev and remove blk_ptr
  //     while (walker[0] != blk_val) walker = addr_v_to_p(walker[0]);
  //     walker[0] = blk_ptr[0];
  //   }
  //   // Replace content with new
  //   blk_ptr[0] = 0;
  //   // Check if break
  //   if (blk_siz >= (rsize + HEADER_SIZE + MIN_BLOCK_SIZE)) {
  //     blk_ptr[-1] = header_pack(tag, rsize);
  //     // Get remaining free mem after new header
  //     blk_siz = blk_siz - rsize - HEADER_SIZE;
  //     walker = blk_ptr + rsize + HEADER_SIZE;
  //     walker[-1] = header_pack(tag_None, blk_siz);
  //     // Insert into free list
  //     walker[0] = addr_p_to_v(free_list_heads[FREE_LISTS_COUNT-1]);
  //     free_list_heads[FREE_LISTS_COUNT-1] = walker;
  //   }
  //   else {
  //     blk_ptr[-1] = header_pack(tag, blk_siz);
  //   }
  //   bitmap_set_bit(blk_ptr);
  // }
  // return blk_ptr;
}

static void mark(value_t* block) {
  // TODO
  if (block < heap_start || block >= heap_end || !bitmap_is_bit_set(block)) return;
  // Clear bit
  bitmap_clear_bit(block);
  value_t blk_siz = header_unpack_size(block[-1]);
  value_t blk_val = 0;
  value_t *walker = NULL;
  // Check for references to other chunks
  for (int i = 0; i < blk_siz; i++) {
    // Get value
    blk_val = block[i];
    if ((blk_val & 0x03) == 0 && heap_start_v <= blk_val && blk_val < heap_end_v) { // Check valid vaddr 
      walker = addr_v_to_p(blk_val);
      // Check in bounds and valid block
      if (bitmap_is_bit_set(walker))
        mark(walker);
    }
  }
}

static void sweep() {
  // TODO
  // Reset free list
  unsigned int findx = 0;
  value_t blk_siz = 0;
  value_t blk_val = 0;
  value_t *walker = NULL;
  value_t *blk_ptr = NULL;
  for (int l = 0; l < FREE_LISTS_COUNT; ++l) {
    // Mark to free
    walker = free_list_heads[l];
    while (walker != memory_start) {
      bitmap_set_bit(walker);
      walker = addr_v_to_p(walker[0]);
    }
    free_list_heads[l] = memory_start;
  }
  // Start from beginning of heap find blocks
  walker = heap_first_block;
  while (walker < heap_end) {
    // Flip bit
    if (bitmap_is_bit_set(walker)) {
      bitmap_clear_bit(walker);
      blk_siz = header_unpack_size(walker[-1]);
      // Can coalesce
      if (blk_val) {
        free_list_heads[findx] = addr_v_to_p(blk_ptr[0]);
        blk_siz = blk_siz + header_unpack_size(blk_ptr[-1]) + HEADER_SIZE;
        blk_ptr[-1] = header_pack(tag_None, blk_siz);
        walker = blk_ptr;
      }
      // Add to free list
      findx = free_list_index(blk_siz);
      walker[0] = addr_p_to_v(free_list_heads[findx]);
      free_list_heads[findx] = walker;
      blk_ptr = walker;
      // Setting previous was freed
      blk_val = 1;
    }
    else {
      // Nothing
      bitmap_set_bit(walker);
      blk_val = 0;
    }
    // Get next
    walker = walker + header_unpack_size(walker[-1]) + HEADER_SIZE;
  }

  // Single list
  // value_t blk_siz = 0;
  // value_t blk_val = 0;
  // value_t *blk_ptr = NULL;
  // value_t *walker = free_list_heads[FREE_LISTS_COUNT-1];
  // while (walker != memory_start) {
  //   bitmap_set_bit(walker);
  //   walker = addr_v_to_p(walker[0]);
  // }
  // free_list_heads[FREE_LISTS_COUNT-1] = memory_start;
  // walker = heap_first_block;
  // while (walker < heap_end) {
  //   blk_siz = header_unpack_size(walker[-1]);
  //   // Flip bit
  //   if (bitmap_is_bit_set(walker)) {
  //     bitmap_clear_bit(walker);
  //     // Can coalesce
  //     if (blk_val) {
  //       free_list_heads[FREE_LISTS_COUNT-1] = addr_v_to_p(blk_ptr[0]);
  //       blk_siz += header_unpack_size(blk_ptr[-1]) + HEADER_SIZE;
  //       blk_ptr[-1] = header_pack(tag_None, blk_siz);
  //       walker = blk_ptr;
  //     }
  //     // Add to free list
  //     walker[0] = addr_p_to_v(free_list_heads[FREE_LISTS_COUNT-1]);
  //     free_list_heads[FREE_LISTS_COUNT-1] = walker;
  //     blk_ptr = walker;
  //     // Setting previous was freed
  //     blk_val = 1;
  //   }
  //   else {
  //     // Nothing
  //     bitmap_set_bit(walker);
  //     blk_val = 0;
  //   }
  //   // Get next
  //   walker += blk_siz + HEADER_SIZE;
  // }
}

value_t* memory_allocate(tag_t tag, value_t size) {
  value_t* first_try = allocate(tag, size);
  if (first_try != NULL)
    return first_try;

  value_t* lb = engine_get_Lb();
  if (lb != memory_start) mark(lb);
  value_t* ib = engine_get_Ib();
  if (ib != memory_start) mark(ib);
  value_t* ob = engine_get_Ob();
  if (ob != memory_start) mark(ob);

  sweep();

  value_t* second_try = allocate(tag, size);
  if (second_try != NULL)
    return second_try;

  fail("\ncannot allocate %d words of memory, even after GC\n", size);
}

value_t memory_get_block_size(value_t* block) {
  return header_unpack_size(block[-1]);
}

tag_t memory_get_block_tag(value_t* block) {
  return header_unpack_tag(block[-1]);
}

#endif
