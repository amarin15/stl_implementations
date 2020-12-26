#pragma once

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
        - this points to the size (not the unallocated space)

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

void* malloc(size_t size)
{
    return NULL;
}

void free(void* ptr)
{
    (void)ptr;
}

} // namespace si
