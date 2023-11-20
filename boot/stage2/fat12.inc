%ifndef FAT12_INC
%define FAT12_INC

bits 16

%include "floppy16.inc"

%define ROOT_OFFSET 0x2e00
%define FAT_SEG     0x02c0
%define ROOT_SEG    0x02e0

; ==========================
; Loads root directory table
; ==========================

LoadRoot:
  pusha
  push es

  ; ---------------------------------
  ; Compute root directory size => CX
  ; ---------------------------------

  xor  cx, cx
  xor  dx, dx
  mov  ax, 32                   ; 32 B directory entry
  mul  WORD [bpbRootEntries]    ; Directory size in B
  div  WORD [bpbBytesPerSector] ; Sectors used by directory
  xchg ax, cx

  ; -------------------------------------
  ; Compute root directory location => AX
  ; -------------------------------------

  mov  al, BYTE [bpbNumberOfFATs]    ; Number of FATs
  mul  WORD [bpbSectorsPerFAT]       ; Sectors used by FATs
  add  ax, WORD [bpbReservedSectors] ; Adjust for bootsector
  mov  WORD [dataSector], ax         ; Start of root directory
  add  WORD [dataSector], cx

  ; -----------------------------
  ; Load root directory at 0x7E00
  ; -----------------------------

  push WORD ROOT_SEG
  pop  es
  mov  bx, 0       ; Copy root directory table
  call ReadSectors ; Read root directory table
  pop  es
  popa
  ret

; =================================
; Loads File Allocation Table (FAT)
; =================================

LoadFAT:
  pusha
  push  es

  ; ----------------------
  ; Compute FAT size => CX
  ; ----------------------

  xor ax, ax
  mov al, BYTE [bpbNumberOfFATs]
  mul WORD [bpbSectorsPerFAT]
  mov cx, ax

  ; --------------------------
  ; Compute FAT location => AX
  ; --------------------------

  mov ax, WORD [bpbReservedSectors]

  ; ------------------------------------------
  ; Load FAT at 0x7c00 (Overwrites bootsector)
  ; ------------------------------------------

  push WORD FAT_SEG
  pop  es
  xor  bx, bx
  call ReadSectors
  pop  es
  popa
  ret

; ==============================================
; Searchs for a file in the root directory
;
; DS:SI: File name
;
; AX: File index in directory table, -1 on error
; ==============================================

FindFile:
  push cx
  push dx
  push bx
  mov  bx, si ; Store file name

  ; --------------------------------------
  ; Search root directory for binary image
  ; --------------------------------------

  mov cx, WORD [bpbRootEntries]
  mov di, ROOT_OFFSET ; First root entry at 1 MiB mark
  cld

  .Loop:
    push cx
    mov  cx, 11 ; 11 character file mane
    mov  si, bx ; Image name
    push di
    rep cmpsb   ; Check for match
    pop  di
    je   .Found
    pop  cx
    add  di, 32 ; Next directory entry
    loop .Loop
  .NotFound:
    pop bx
    pop dx
    pop cx
    mov ax, -1 ; Error code
    ret
  .Found:
    pop ax ; File entry
    pop bx
    pop cx
    ret

; ==============================
; Loads a file
;
; ES:SI:  File to load
; EBX:BP: Buffer to load file to
;
; AX: 0 on success, -1 on error
; CX: Sectors read
; ==============================

LoadFile:
  xor  ecx, ecx ; File size in sectors
  push ecx
  .FindFile:
    push bx ; Store pointer to buffer
    push bp
    call FindFile
    cmp  ax, -1
    jne  .PrepareLoadImage
    pop  bp
    pop  bx
    pop  ecx
    mov  ax, -1
    ret
  .PrepareLoadImage:
    sub edi, ROOT_OFFSET
    sub eax, ROOT_OFFSET
    ; Get starting cluster
    push WORD ROOT_SEG
    pop  es
    mov  dx, WORD [es:di + 0x001A] ; DI: first directory entry
    mov  WORD [cluster], dx ; First file cluster
    pop  bx
    pop  es
    push bx ; Store location
    push es
    call LoadFAT
  .LoadImage:
    ; Load cluster
    mov  ax, WORD [cluster] ; Cluster to read
    pop  es
    pop  bx
    call CHSToLBA
    xor  cx, cx
    mov  cl, BYTE [bpbSectorsPerCluster]
    call ReadSectors
    pop  ecx
    inc  ecx ; Adjust for bootsector
    push ecx
    push bx
    push es
    mov  ax, FAT_SEG ; Start reading from FAT
    mov  es, ax
    xor  bx, bx
    ; Get next cluster
    mov  ax, WORD [cluster] ; Current cluster
    mov  cx, ax
    mov  dx, ax
    shr  dx, 0x0001         ; Divide by 2
    add  cx, dx             ; Sum for (3/2)
    mov  bx, 0              ; FAT memory location
    add  bx, cx
    mov  dx, WORD [es:bx]
    test ax, 0x0001        ; Test for odd/even cluster
    jnz  .OddCluster
  .EvenCluster:
    and dx, 0x0fff ; Low  12 bits
    jmp .Done
  .OddCluster:
    shr dx, 0x0004     ; High 12 bits
  .Done:
    mov WORD [cluster], dx
    cmp dx, 0x0ff0 ; Check for EOF
    jb  .LoadImage
  .Success:
    pop es
    pop bx
    pop ecx
    xor ax, ax
    ret

%endif
