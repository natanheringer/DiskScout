section .text 
    global quick_add
    global compare_sizes 

; void quick_add(uint64_t *total, uint64_t value)
; ultra fast sum of total values (thread-safe for multi-million files)
; Windows x64 calling convention: rcx = pointer to total, rdx = value to add
quick_add:
    lock add qword [rcx], rdx        ; atomic *total += value
    ret 

; int compare_sizes(const void *a, const void *b)
; compare sizes of two structs DirInfo to qsort 
; decrecent order (bigger first)
; Windows x64 calling convention: rcx = DirInfo pointer a, rdx = DirInfo pointer b
; return: rax = difference (if a > b = negative, if b > a = positive)
compare_sizes: 
    ; 'size' field offset in DirInfo struct 
    ; char path[4096] + uint64_t size
    ; size is in offset 4096

    mov rax, [rdx + 4096]           ; rax = b->size 
    sub rax, [rcx + 4096]           ; rax = b->size - a->size
    ret                             ; returns the difference
