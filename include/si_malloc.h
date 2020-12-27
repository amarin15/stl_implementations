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

    - the size of each chunk includes the overhead
    - allocated memory is 8-byte aligned
        - allows us to reuse the 3 least significant bits in `size`
        - these form the control mask:
            - C (size & 0x02) = current chunk is free
            - P (size & 0x01) = previous chunk is free
            - size & 0x04 can potentially be used later for optimizations such as using
              mmap for large allocations.
    - the minimum chunk size is 16 bytes on 32-bit systems and 24 bytes on 64-bit systems
    - only free chunks have a pointer to next free chunk
        - this points to the first size (not the unallocated space)

- malloc algorithm
    - start with pointer to first free chunk
    - go through each free chunk using next pointers
        - find the first available space (first fit)
    - if not enough space, use sbrk
    - split the found chunk into allocated and free

- free algorithm
    - assume this is an allocated chunk (otherwise undefined behaviour)
    - check if we can merge with the previous chunk
        - use the P bit in the size
    - check if we can merge next chunk
        - use pointer to next free chunk, see if it's the same as the next chunk
    - merge if needed
        - update size at the beginning and end
        - update previous free chunk's next pointer

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
#define MIN_ALIGNMENT   8
#define CONTROL_MASK    0x07

// Pointer to the address of the first free chunk
static CHUNK_ADDR_T* first_free_chunk = NULL;
static CHUNK_ADDR_T* last_free_chunk = NULL;

// Ensures alignment
inline size_t next_aligned(size_t size)
{
    return size + ((MIN_ALIGNMENT - (size & CONTROL_MASK)) & CONTROL_MASK);
}

// Returns a pointer to the address of the next available free chunk
inline CHUNK_ADDR_T* next_free(CHUNK_ADDR_T free_chunk)
{
    return (CHUNK_ADDR_T*)(static_cast<char*>(free_chunk) + sizeof(CHUNK_SIZE_T));
}

inline CHUNK_SIZE_T get_size_from_beginning(CHUNK_ADDR_T free_chunk)
{
    return *(static_cast<CHUNK_SIZE_T *>(free_chunk));
}

// Assumes the previous chunk is a free chunk
inline CHUNK_SIZE_T get_size_from_prev_end(CHUNK_ADDR_T chunk)
{
    CHUNK_ADDR_T prev_size_addr = (CHUNK_ADDR_T)(static_cast<char*>(chunk) - sizeof(CHUNK_SIZE_T));
    return *(static_cast<CHUNK_SIZE_T *>(prev_size_addr));
}

// Sets the size of the chunk at the beginning only.
inline void set_size_at_beginning(CHUNK_ADDR_T chunk, size_t size)
{
    *(size_t*)(chunk) = size;
}

// Sets the size of the free chunk at the beginning and at the end.
inline void set_size_free_chunk(CHUNK_ADDR_T free_chunk, size_t size)
{
    set_size_at_beginning(free_chunk, size);
    *(size_t*)(static_cast<char*>(free_chunk) + size - sizeof(CHUNK_SIZE_T)) = size;
}

inline void mark_last_free(CHUNK_ADDR_T free_chunk)
{
    *next_free(free_chunk) = NULL;
    last_free_chunk = &free_chunk;
}


// Checks if the previous free chunk ends at the position where the current chunk starts.
// If so, it means the previous chunk is free and updates the P bit in size of the current chunk.
void update_chunk_P_bit(CHUNK_ADDR_T chunk, CHUNK_SIZE_T p_bit_mask)
{
    // First confirm that we are not at the end of the heap
    CHUNK_ADDR_T const program_break = sbrk(0);
    assert(chunk <= program_break);
    if (chunk == program_break)
        return;

    // Read the size and mask of the current chunk
    CHUNK_SIZE_T size = get_size_from_beginning(chunk);

    // If the P bit already had the same value, there is nothing to do
    if ((size & 0x01) == p_bit_mask)
        return;

    // Update the P bit
    size |= p_bit_mask;

    if (size & 0x02) // if this is a free chunk
        set_size_free_chunk(chunk, size);
    else
        set_size_at_beginning(chunk, size);
}

// Checks if the previous free chunk ends at the position where the current chunk starts.
// If so, it means the previous chunk is free and updates the P bit in the mask of the current chunk.
void update_mask_P_bit(CHUNK_ADDR_T prev_free_chunk, CHUNK_ADDR_T cur_chunk, CHUNK_SIZE_T& cur_mask)
{
    if (prev_free_chunk)
    {
        const CHUNK_SIZE_T prev_size = get_size_from_beginning(prev_free_chunk) & ~CONTROL_MASK;
        CHUNK_ADDR_T const prev_end = (CHUNK_ADDR_T const)(static_cast<char*>(prev_free_chunk) + prev_size);

        if (prev_end == cur_chunk)
            cur_mask |= 0x01;
    }
}

// The mask contains the 2 least significant bits C and P
CHUNK_ADDR_T alloc_sbrk(size_t size, size_t mask)
{
    // If SBRK_ALLOC_SIZE > size, we only want to create another free chunk from
    // the extra space if there is enough memory to hold the new free chunk.
    size_t allocated_size = size;
    const size_t min_free_chunk_size = MIN_ALLOC_SIZE + sizeof(CHUNK_SIZE_T);
    if (SBRK_ALLOC_SIZE > size + min_free_chunk_size)
        allocated_size = SBRK_ALLOC_SIZE;

    void* allocated = sbrk(allocated_size);

    if (allocated == (void*)-1)
        return NULL;

    // Create a free chunk from the allocated memory
    set_size_free_chunk(allocated, allocated_size | mask);
    mark_last_free(allocated);

    return allocated;
}

// Assumes that cur_chunk has enough space to hold the requested size.
// Potentially breaks cur_chunk into an allocated chunk and a free chunk.
//  Adds the free chunk to the free chunk list and returns the address of
//  the allocated space in the allocated chunk.
void* split(CHUNK_ADDR_T cur_chunk, size_t required_size, CHUNK_ADDR_T prev_free_chunk)
{
    // Confirm that we have enough space for the allocated chunk.
    // The chunk size also includes the overhead.
    CHUNK_SIZE_T chunk_size = get_size_from_beginning(cur_chunk) & ~CONTROL_MASK;
    assert(chunk_size >= required_size);

    // Set the size and mask of the allocated chunk.
    // C will 0 because it's an allocated chunk.
    CHUNK_SIZE_T allocated_size_mask = 0x00;
    // To find P, we check if previous free chunk is also the previous chunk.
    update_mask_P_bit(prev_free_chunk, cur_chunk, allocated_size_mask);
    set_size_at_beginning(cur_chunk, required_size | allocated_size_mask);

    // If this was the only free chunk, update first_free_chunk
    const bool was_first_free_chunk = (cur_chunk == *first_free_chunk);
    if (was_first_free_chunk)
        first_free_chunk = next_free(*first_free_chunk);

    // The current chunk is not free, so next's P bit will be 0
    CHUNK_SIZE_T next_P_bit = 0x00;
    bool updated_prev_free_next = false;
    if (chunk_size > required_size)
    {
        // Try to create a free chunk from the remaining space
        const size_t remaining_size = chunk_size - required_size;
        // Try to fit a free chunk, otherwise this space will be wasted
        if (remaining_size >= MIN_ALLOC_SIZE + sizeof(CHUNK_SIZE_T))
        {
            // The next chunk will have a free chunk as the previous chunk
            next_P_bit = 0x01;

            // Set the size including the overhead of the new free chunk
            CHUNK_ADDR_T free_chunk = (CHUNK_ADDR_T)(static_cast<char*>(cur_chunk) + required_size);
            // C will 1 because it's a free chunk.
            // P will be 0, because the previous chunk is allocated.
            const CHUNK_SIZE_T free_chunk_size_mask = 0x02;
            set_size_free_chunk(free_chunk, remaining_size | free_chunk_size_mask);

            // Set the pointer to the next free chunk
            *next_free(free_chunk) = *next_free(cur_chunk);

            // Update the next pointer of the previous free chunk
            if (prev_free_chunk != NULL)
                *next_free(prev_free_chunk) = free_chunk;
            updated_prev_free_next = true;

            if (was_first_free_chunk)
                first_free_chunk = &free_chunk;
        }
    }

    // Update the next pointer of the previous last free chunk if we haven't already
    if (!updated_prev_free_next && prev_free_chunk != NULL)
        *next_free(prev_free_chunk) = *next_free(cur_chunk);

    // Update the P pointer of the next chunk
    CHUNK_ADDR_T next_chunk = (CHUNK_ADDR_T)(static_cast<char*>(cur_chunk) + chunk_size);
    update_chunk_P_bit(next_chunk, next_P_bit);

    void* user_allocated = (void*)(static_cast<char*>(cur_chunk) + sizeof(CHUNK_SIZE_T));
    return user_allocated;
}

void* malloc(size_t size)
{
    if (size == 0)
        return NULL;

    // Ensure we allocate at least the minimum space required to hold a free chunk.
    // We want all chunks to have be aligned.
    if (size < MIN_ALLOC_SIZE)
        size = MIN_ALLOC_SIZE;
    const size_t required_size = next_aligned(size + sizeof(CHUNK_SIZE_T));

    // If there is no free memory, try to allocate and return if system fails
    if (! first_free_chunk)
    {
        // C is 1 because the new chunk will be free
        // P is 0 because there are no other free chunks
        CHUNK_ADDR_T chunk = alloc_sbrk(required_size, 0x02);
        if (chunk == NULL)
            return NULL;

        first_free_chunk = &chunk;
    }

    // Find the first free chunk that contains enough memory to fit the request
    CHUNK_ADDR_T cur_free = *first_free_chunk;
    CHUNK_ADDR_T prev_free = NULL;
    while (get_size_from_beginning(cur_free) < required_size)
    {
        prev_free = cur_free;
        cur_free = *next_free(cur_free);

        // If this is the last chunk, try to allocate memory and return if system fails
        if (cur_free == NULL)
        {
            // C is 1 because the new chunk will be free.
            CHUNK_SIZE_T mask = 0x02;
            // To find P (if the previous chunk was free), we check if prev_free is also
            // the previous chunk (last one, since we are allocating at the end).
            CHUNK_ADDR_T const program_break = sbrk(0);
            update_mask_P_bit(prev_free, program_break, mask);

            cur_free = alloc_sbrk(required_size, mask);
            if (cur_free == NULL)
                return NULL;

            // TODO: Update the next pointer of the previous last free chunk
            break;
        }
    }

    return split(cur_free, required_size, prev_free);
}

// Undefined behaviour if we call free on a pointer that was not returned
// with malloc or that was previously freed.
void free(void* ptr)
{
    if (!ptr)
        return;

    // Check if we can coalesce the previous chunk
    CHUNK_ADDR_T chunk = (CHUNK_ADDR_T)(static_cast<char*>(ptr) - sizeof(CHUNK_SIZE_T));
    const CHUNK_SIZE_T chunk_size = get_size_from_beginning(chunk);

    CHUNK_ADDR_T coalesced_start = chunk;
    CHUNK_SIZE_T coalesced_size = chunk_size & ~CONTROL_MASK;
    if (chunk_size & 0x01)
    {
        const CHUNK_SIZE_T prev_size = get_size_from_prev_end(chunk) & ~CONTROL_MASK;
        coalesced_start = (CHUNK_ADDR_T)(static_cast<char*>(chunk) - prev_size);
        coalesced_size += prev_size;
    }

    // Check if we can coalesce the next chunk
    CHUNK_ADDR_T next_chunk = (CHUNK_ADDR_T)(static_cast<char*>(coalesced_start) + coalesced_size);
    CHUNK_ADDR_T const program_break = sbrk(0);
    assert(next_chunk <= program_break);

    CHUNK_ADDR_T coalesced_end = next_chunk;
    if (next_chunk < program_break)
    {
        CHUNK_SIZE_T next_size = get_size_from_beginning(next_chunk);
        if (next_size & 0x02)
        {
            // remove the mask
            next_size = next_size & ~CONTROL_MASK;
            // update the coalesced end and size
            coalesced_end = (CHUNK_ADDR_T)(static_cast<char*>(coalesced_end) + next_size);
            coalesced_size += next_size;
        }
    }

    // Update the metadata of the coalesced chunk.
    // C is 1 because the current chunk is free.
    // P is 0 because if the previous chunk were free, we would have coalesced it.
    set_size_free_chunk(coalesced_start, coalesced_size | 0x02);

    // Set the P bit of the next chunk because this is a free chunk
    update_chunk_P_bit(coalesced_end, 0x01);

    // Update first_free_chunk and last_free_chunk
    // and the current chunk's next free pointer
    if (!first_free_chunk)
    {
        first_free_chunk = &coalesced_start;
        mark_last_free(first_free_chunk);
        *next_free(coalesced_start) = NULL;
    }
    else if (coalesced_start < *first_free_chunk)
    {
        *next_free(coalesced_start) = *first_free_chunk;
        first_free_chunk = &coalesced_start;
    }
    else if (coalesced_start == *first_free_chunk)
    {
        *next_free(coalesced_start) = *next_free(*first_free_chunk);
        if (last_free_chunk == first_free_chunk)
            last_free_chunk = &coalesced_start;
        first_free_chunk = &coalesced_start;
    }
    else // coalesced_start > *first_free_chunk
    {
        // The last free chunk can't be NULL since the first_free_chunk wasn't NULL
        assert(last_free_chunk != NULL);

        // Update the previous free chunk's next pointer.
        // Maybe this could be improved so we don't have to walk the entire free chunk list.
        CHUNK_ADDR_T* cur = next_free(*first_free_chunk);
        CHUNK_ADDR_T* prev = first_free_chunk;
        bool updated_prev_free_next = false;
        while (cur)
        {
            if (*cur > coalesced_start)
            {
                *next_free(*prev) = coalesced_start;
                updated_prev_free_next = true;
                break;
            }

            prev = cur;
            cur = next_free(*cur);
        }

        // If this is the last free chunk
        if (!updated_prev_free_next)
        {
            *next_free(*prev) = coalesced_start;
            mark_last_free(coalesced_start);
        }
    }
}

} // namespace si
