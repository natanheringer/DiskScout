section .text 
    global quick_add
    global compare_sizes 

; void quick_add(uint64_t *total, uint64_t value)
; ultra fast sum of total values
; parameters: rdi = pointer to total, rsi = value to add
quick_add:
    add qword [rdi], rsi            ; *total += vaçie (1 instruction!)
    ret 

; int compare_sizes(const void *a, const void *b)
; compare sizes of two structs DirInfo to qsort 
; decrecent order (bigger first)
; Parameters: rdi = DirInfo pointer a, rsi = DirInfo ponter b
; return: rax = difference (if a > b = negative, if b > a = positive)
compare_sizes: 
    ; 'size' field offset in DirInfo struct 
    ; char path[4096] + uint64_t size
    ; size is in offset 4096

    mov rax, [rsi + 4096]           ; rax = b->size 
    sub rax, [rdi + 4096]           ; rax = b->size - a->size
    ret                             ; returns the difference

    