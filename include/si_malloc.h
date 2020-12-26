#pragma once

namespace si {

/*
- the heap space will be split into free and allocated chunks with the following structure:

     allocated chunk -> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                        | size of chunk, in bytes                                       |
                        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                        | (1 byte) control byte                                         |
                        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                        | allocated  space                                              |
                        |                                                               |
                        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                        | size of chunk, in bytes                                       |
                        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                        | (1 byte) control byte                                         |
          free chunk -> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                        | size of chunk, in bytes                                       |
                        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                        | (1 byte) control byte                                         |
                        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                        | pointer to next free chunk                                    |
                        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                        | unallocated space                                             |
                        |                                                               |
                        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                        | size of chunk, in bytes                                       |
                        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                        | (1 byte) control byte                                         |
                        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

    - the value of the control byte can be
        - bit 7 = is free chunk
        - bit 6 = is first chunk
        - bit 5 = is last chunk
    - only free chunks have a pointer to next free chunk
        - this points to the size (not the unallocated space)

- malloc (size)
    - start with pointer to first free chunk
    - go through each free chunk using next pointers
        - find the first available space (first fit)
    - if not enough space
        - if allocation is bigger than 1mb, use mmap
        - else use sbrk
    - split the chunk into allocated and free

- free (addr)
    - assume this is an allocated chunk (otherwise undefined behaviour)
    - read size from addr sizeof(ptr) - 1 - sizeof(size)
    - check if we can merge with the previous chunk
        - read the control byte
        - if not first chunk
            - read the previous control byte
            - can_merge_prev = is prev free chunk
    - check if we can merge next chunk
        - if not last chunk
            - read control of next chunk
            - can_merge_next = is next free chunk
    - update size and control bytes depending on can_merge_prev and can_merge_next
    - optimization: if we have a large free chunk at the end, we can release it
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
