section .text 
    global quick_add
    global compare_sizes 
    global fast_strcmp_dot
    global fast_strcmp_dotdot
    global atomic_inc_file_count
    global fast_should_skip
    global fast_wstrcmp_dot
    global fast_wstrcmp_dotdot
    global fast_wshould_skip
    global fast_path_copy

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

; int fast_strcmp_dot(const char *str)
; fast check if string equals "."
; Windows x64: rcx = string pointer
; Returns: rax = 1 if equal to ".", 0 otherwise
fast_strcmp_dot: 
    cmp byte [rcx], '.'             ; Check first character
    jne not_dot    
    cmp byte [rcx + 1], 0           ; Check if null terminator
    jne not_dot
    mov rax, 1                      ; return 1 (equal)
    ret
not_dot:
    mov rax, 0                      ; return 0 (not equal)
    ret

; int fast_strcmp_dotdot(const char *str)
; fast check if string equals ".."
; Windows x64: rcx = string pointer
; Returns: rax = 1 if equal to "..", 0 otherwise
fast_strcmp_dotdot:
    cmp byte [rcx], '.'             ; check first character
    jne not_dotdot
    cmp byte [rcx + 1], '.'         ; check second character
    jne not_dotdot
    cmp byte [rcx + 2], 0           ; check null terminator
    jne not_dotdot
    mov rax, 1                      ; return 1 (equal)
    ret

not_dotdot: 
    mov rax, 0                      ; return 0 (not equal)
    ret

; ========================= Wide-char helpers (UTF-16) =========================
; int fast_wstrcmp_dot(const wchar_t *str)
; rcx = wide string pointer
; Returns rax=1 if equals L".", else 0
fast_wstrcmp_dot:
    movzx eax, word [rcx]            ; first wide char
    cmp ax, 0x002E                   ; '.'
    jne .w_not
    movzx eax, word [rcx+2]
    test ax, ax
    jne .w_not
    mov rax, 1
    ret
.w_not:
    xor rax, rax
    ret

; int fast_wstrcmp_dotdot(const wchar_t *str)
; rcx = wide string pointer
; Returns rax=1 if equals L"..", else 0
fast_wstrcmp_dotdot:
    movzx eax, word [rcx]
    cmp ax, 0x002E                   ; '.'
    jne .ww_not
    movzx eax, word [rcx+2]
    cmp ax, 0x002E                   ; '.'
    jne .ww_not
    movzx eax, word [rcx+4]
    test ax, ax                      ; terminator
    jne .ww_not
    mov rax, 1
    ret
.ww_not:
    xor rax, rax
    ret

; void atomic_inc_file_count(int *file_count)
; Atomic increment of file counter without mutex
; Windows x64: rcx = pointer to file_count
atomic_inc_file_count: 
    lock inc dword[rcx]             ; atomic (*file_count)++
    ret 

; int fast_should_skip(const char *name)
; fast dffirectory name chacking against skip list
; Windows x64: rcx = directory name pointer 
; returns: rax = 1 if should skip, 0 otherwise 
fast_should_skip:
    ; Windows x64: preserve non-volatile regs and provide shadow space
    push rsi                        ; preserve callee-saved rsi
    sub  rsp, 40h                   ; 32-byte shadow space + align for calls

    ; node_modules
    mov  rdx, rcx                   ; rdx = name
    lea  rsi, [rel node_modules_str]
    call strcmp_impl
    test rax, rax
    je   .skip

    ; .git
    mov  rdx, rcx
    lea  rsi, [rel git_str]
    call strcmp_impl
    test rax, rax
    je   .skip

    ; .svn
    mov  rdx, rcx
    lea  rsi, [rel svn_str]
    call strcmp_impl
    test rax, rax
    je   .skip

    ; .cache
    mov  rdx, rcx
    lea  rsi, [rel cache_str]
    call strcmp_impl
    test rax, rax
    je   .skip

    ; __pycache__
    mov  rdx, rcx
    lea  rsi, [rel pycache_str]
    call strcmp_impl
    test rax, rax
    je   .skip

    add  rsp, 40h
    pop  rsi
    xor  rax, rax                   ; don't skip
    ret

.skip:
    add  rsp, 40h
    pop  rsi
    mov  rax, 1                     ; skip
    ret

; Internal string comparison helper
; rdx = str1, rsi = str2 
strcmp_impl:
    ; Provide shadow space and preserve rbx (non-volatile)
    push rbx
    sub  rsp, 20h               ; shadow space for potential calls (safety)
.loop:
    mov  al, [rdx]
    mov  bl, [rsi]
    cmp  al, bl
    jne  .ne
    test al, al
    jz   .eq
    inc  rdx
    inc  rsi
    jmp  .loop
.eq:
    add  rsp, 20h
    pop  rbx
    xor  rax, rax
    ret
.ne:
    add  rsp, 20h
    pop  rbx
    mov  rax, 1
    ret

; void fast_path_copy(char *dest, const char *src, size_t max_len)
; Fast path copying with length limit
; Windows x64: rcx = dest, rdx = src, r8 = max_len
fast_path_copy:
    test r8, r8                     ; Check if max_len is 0
    jz copy_done
    dec r8                          ; Reserve space for null terminator
    
copy_loop:
    test r8, r8                     ; Check remaining length
    jz copy_done
    mov al, [rdx]                   ; Load source byte
    mov [rcx], al                   ; Store to destination
    test al, al                     ; Check for null terminator
    jz copy_done
    inc rcx
    inc rdx
    dec r8
    jmp copy_loop
    
copy_done:
    mov byte [rcx], 0               ; Ensure null termination
    ret

; String constants for skip list
node_modules_str: db "node_modules", 0
git_str: db ".git", 0
svn_str: db ".svn", 0
cache_str: db ".cache", 0
pycache_str: db "__pycache__", 0

; ======================= Wide-char skiplist (UTF-16) ==========================
; int fast_wshould_skip(const wchar_t *name)
; rcx = wide string pointer
; returns rax=1 to skip, 0 otherwise
fast_wshould_skip:
    push rsi
    sub  rsp, 40h
    ; L"node_modules"
    mov  rdx, rcx
    lea  rsi, [rel w_node_modules]
    call wstrcmp_impl
    test rax, rax
    je   .w_skip
    ; L".git"
    mov  rdx, rcx
    lea  rsi, [rel w_git]
    call wstrcmp_impl
    test rax, rax
    je   .w_skip
    ; L".svn"
    mov  rdx, rcx
    lea  rsi, [rel w_svn]
    call wstrcmp_impl
    test rax, rax
    je   .w_skip
    ; L".cache"
    mov  rdx, rcx
    lea  rsi, [rel w_cache]
    call wstrcmp_impl
    test rax, rax
    je   .w_skip
    ; L"__pycache__"
    mov  rdx, rcx
    lea  rsi, [rel w_pycache]
    call wstrcmp_impl
    test rax, rax
    je   .w_skip
    add  rsp, 40h
    pop  rsi
    xor  rax, rax
    ret
.w_skip:
    add  rsp, 40h
    pop  rsi
    mov  rax, 1
    ret

; wide strcmp: returns rax=0 if equal, 1 if not equal
wstrcmp_impl:
    push rbx
    sub  rsp, 20h
.wloop:
    movzx eax, word [rdx]
    movzx ebx, word [rsi]
    cmp  ax, bx
    jne  .wne
    test ax, ax
    jz   .weq
    add  rdx, 2
    add  rsi, 2
    jmp  .wloop
.weq:
    add  rsp, 20h
    pop  rbx
    xor  rax, rax
    ret
.wne:
    add  rsp, 20h
    pop  rbx
    mov  rax, 1
    ret

; UTF-16 string literals (dw: 16-bit code units)
w_node_modules: dw 'n','o','d','e','_','m','o','d','u','l','e','s',0
w_git:          dw '.', 'g','i','t',0
w_svn:          dw '.', 's','v','n',0
w_cache:        dw '.', 'c','a','c','h','e',0
w_pycache:      dw '_','_','p','y','c','a','c','h','e','_','_',0