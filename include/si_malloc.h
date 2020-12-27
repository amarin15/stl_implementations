#pragma once

#include <unistd.h>
#include <assert.h>

namespace si {

/*
First-fit with coalescing malloc implementation.

- the heap space will be split into free and allocated chunks with the following structure:

     allocated chunk -> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                        | size of chunk, in bytes                                   |C|P|
                        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                        | allocated space                                               |
                        |                                                               |
          free chunk -> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                        | size of chunk, in bytes                                   |C|P|
                        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                        | pointer to next free chunk                                    |
                        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                        | unallocated space                                             |
                        |                                                               |
                        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                        | size of chunk, in bytes                                   |C|P|
                        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

    - allocated memory is 8-byte aligned
        - this allows us to reuse the 3 least significant bits in `size`
        - C (size & 0x02) = current chunk is free
        - P (size & 0x01) = previous chunk is free
        - size & 0x04 can potentially be used later for is mmap-ed
    - the minimum chunk size is 16 bytes on 32-bit systems and 24 bytes on 64-bit systems
    - only free chunks have a pointer to next free chunk
        - this points to the first size (not the unallocated space)
    - the size of each chunk includes the overhead

- malloc (size)
    - start with pointer to first free chunk
    - go through each free chunk using next pointers
        - find the first available space (first fit)
    - if not enough space, use sbrk
    - split the chunk into allocated and free

- free (addr)
    - assume this is an allocated chunk (otherwise undefined behaviour)
    - read size from addr sizeof(ptr) - 1 - sizeof(size)
    - check if we can merge with the previous chunk
        - read the control byte
        - can_merge_prev = is prev free chunk
    - check if we can merge next chunk
        - if not last chunk (compare with sbrk(0))
            - read control of next chunk
            - can_merge_next = is next free chunk
    - update size and control bytes depending on can_merge_prev and can_merge_next

- potential optimizations
    - use mmap, munmap if the allocation is bigger than 1mb
        - be careful to not coalesce mmap-ed chunks with brk-ed chunks (they might not be contiguous)
    - http://gee.cs.oswego.edu/dl/html/malloc.html
        - release the last chunk with sbrk(negative_value) if above a certain threshold
        - hold free chunks in bins sorted by size
            - each bin is a linked list of free chunks
            - elements in each bin can also be sorted (for best-fit instead of first-fit)
        - locality preservation (try to put chunks that were allocated around the same time close to each other)
        - deferred coalescing
            - new allocations of the same size can just reuse the current free chunk and avoid a later split
            - good for cases when a memory frequently allocates and deallocates memory of few sizes
    - https://sourceware.org/glibc/wiki/MallocInternals
        - use multiple arenas for multithreaded environments (to avoid contention on the mutex)
*/


#define CHUNK_SIZE_T    size_t
#define CHUNK_ADDR_T    void *
#define MIN_ALLOC_SIZE  (sizeof(CHUNK_SIZE_T) + sizeof(CHUNK_ADDR_T *))
#define SBRK_ALLOC_SIZE (4096 * 16)

// pointer to the address of the first free chunk
static CHUNK_ADDR_T* first_free_chunk = NULL;

// returns a pointer to the address of the next available free chunk
inline CHUNK_ADDR_T* next_free(CHUNK_ADDR_T free_chunk)
{
    return (CHUNK_ADDR_T*)(static_cast<char*>(free_chunk) + sizeof(CHUNK_SIZE_T));
}

inline CHUNK_SIZE_T get_size_from_beginning(CHUNK_ADDR_T free_chunk)
{
    return *(static_cast<CHUNK_SIZE_T *>(free_chunk));
}

// Set the size of the free chunk at the beginning and at the end
void set_size(CHUNK_ADDR_T free_chunk, size_t size)
{
    *(size_t*)(free_chunk) = size;
    *(size_t*)(static_cast<char*>(free_chunk) + size - sizeof(CHUNK_SIZE_T)) = size;
}

inline void mark_last_free(CHUNK_ADDR_T free_chunk)
{
    *next_free(free_chunk) = NULL;
}

inline size_t max(size_t a, size_t b)
{
    return a > b ? a : b;
}

inline CHUNK_ADDR_T alloc_sbrk(size_t size)
{
    void* allocated = sbrk(max(SBRK_ALLOC_SIZE, size));

    if (allocated == (void*)-1)
        return NULL;

    // Create a chunk from the first size bytes
    set_size(allocated, size);

    return allocated;
}

// Assumes that cur_chunk has enough space to hold the requested size.
// Potentially breaks cur_chunk into an allocated chunk and a free chunk.
//  Adds the free chunk to the free chunk list and returns the address of
//  the allocated space in the allocated chunk.
void* split(CHUNK_ADDR_T cur_chunk, size_t required_size, CHUNK_ADDR_T prev_free_chunk)
{
    // Check for an exact match.
    // The chunk size also includes the overhead.
    const CHUNK_SIZE_T chunk_size = get_size_from_beginning(cur_chunk);
    void* allocated_addr = (void*)(static_cast<char*>(cur_chunk) + sizeof(CHUNK_SIZE_T));
    if (chunk_size == required_size)
        return allocated_addr;

    // Confirm that we have enough space for the allocated chunk.
    assert(chunk_size >= required_size);

    // Try to create a free chunk from the remaining space
    const size_t remaining_size = chunk_size - required_size;
    // If we can't fit a free chunk, this space will be wasted
    if (remaining_size >= MIN_ALLOC_SIZE + sizeof(CHUNK_SIZE_T))
    {
        // Set the size including the overhead of the new free chunk
        CHUNK_ADDR_T free_chunk = (CHUNK_ADDR_T)(static_cast<char*>(cur_chunk) + required_size);
        set_size(free_chunk, remaining_size);

        // Set the pointer to the next free chunk
        *next_free(free_chunk) = *next_free(cur_chunk);

        // Update the next pointer of the previous chunk
        if (prev_free_chunk != NULL)
            *next_free(prev_free_chunk) = free_chunk;
    }

    // Set the size of the allocated chunk and return the address of the allocated space
    *(size_t*)(cur_chunk) = required_size;
    return allocated_addr;
}

void* malloc(size_t size)
{
    if (size == 0)
        return NULL;

    // Ensure we allocate at least the minimum space required to hold a free chunk
    if (size < MIN_ALLOC_SIZE)
        size = MIN_ALLOC_SIZE;
    const size_t required_size = size + sizeof(CHUNK_SIZE_T);

    // Check if there is no free memory
    if (! first_free_chunk)
    {
        // Try to allocate memory and return if system fails
        CHUNK_ADDR_T chunk = alloc_sbrk(required_size);
        if (chunk == NULL)
            return NULL;

        // Also mark the first free chunk as the last free chunk
        mark_last_free(chunk);
        first_free_chunk = &chunk;
    }

    // Find the first free chunk that contains enough memory to fit the request
    CHUNK_ADDR_T cur = *first_free_chunk;
    CHUNK_ADDR_T prev = NULL;
    while (get_size_from_beginning(cur) < required_size)
    {
        prev = cur;
        cur = *next_free(cur);

        // If this is the last chunk
        if (cur == NULL)
        {
            // Try to allocate memory and return if system fails
            cur = alloc_sbrk(required_size);
            if (cur == NULL)
                return NULL;

            break;
        }
    }

    return split(cur, required_size, prev);
}

void free(void* ptr)
{
}

} // namespace si
