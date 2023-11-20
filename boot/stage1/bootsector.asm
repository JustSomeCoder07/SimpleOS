bits   16       ; 16-bit Real Mode
org    0        ; Will be set later
start: jmp Main ; Jump to bootloader entry point

; ====================
; BIOS Parameter Block
; ====================

bpbOEM               DB "SimpleOS"
bpbBytesPerSector    DW 512
bpbSectorsPerCluster DB 1
bpbReservedSectors   DW 1
bpbNumberOfFATs      DB 2
bpbRootEntries       DW 224
bpbTotalSectors      DW 2880
bpbMedia             DB 0xf0 ; 1.44 MB floppy disk
bpbSectorsPerFAT     DW 9
bpbSectorsPerTrack   DW 18
bpbHeadsPerCylinder  DW 2
bpbHiddenSectors     DD 0
bpbTotalSectorsBig   DD 0
bsDriveNumber        DB 0
bsUnused             DB 0
bsExtBootSignature   DB 0x29
bsSerialNumber       DD SERIAL_NUMBER ; Defined at assemble time
bsVolumeLabel        DD VOLUME_LABEL  ; Defined at assemble time
bsFileSystem         DB "FAT12   "

; ============
; I/O routines
; ============

dataSector  DW 0x0000
cluster     DW 0x0000
absoluteSector DB 0x00
absoluteHead   DB 0x00
absoluteTrack  DB 0x00

; --------------------------
; Prints a string
;
; SI: Null-terminated string
; --------------------------

Puts16:
  lodsb
  or    al, al
  jz    .Done ; Check for null terminator
  mov   ah, 0x0e
  int   0x10
  jmp   Puts16
  .Done:
    ret

; -----------------------------------------
; Converts CHS to LBA
;
; LBA = (cluster - 2) * sectors per cluster
;
; AX: Address to convert
; -----------------------------------------

CHSToLBA:
  sub ax, 0x0002 ; Adjust for sector 0
  xor cx, cx
  mov cl, BYTE [bpbSectorsPerCluster]
  mul cx
  add ax, WORD [dataSector] ; Base sector
  ret

; -------------------------------------------------------------------------
; Converts LBA to CHS
;
; Absolute sector = (logical sector /  sectors per track) + 1
; Absolute head   = (logical sector /  sectors per track) % number of heads
; Absolute track  =  logical sector / (sectors per track) * number of heads
;
; AX: Address to convert
; -------------------------------------------------------------------------

LBAToCHS:
  xor dx, dx
  div WORD [bpbSectorsPerTrack]
  inc dl ; Adjust for sector 0
  mov BYTE [absoluteSector], dl
  xor dx, dx
  div WORD [bpbHeadsPerCylinder]
  mov BYTE [absoluteHead], dl
  mov BYTE [absoluteTrack], al
  ret

; --------------------------------
; Reads sectors
;
; CX:    Number of sectors to read
; AX:    First sector
; ES:BX: Buffer to read to
; --------------------------------

ReadSectors:
  .Main:
    mov di, 0x0005 ; 5 retries in case of an error
  .SectorLoop:
    push ax
    push bx
    push cx
    call LBAToCHS    ; Convert first sector to CHS
    mov ah, 0x02     ; BIOS read sector
    mov al, 0x01     ; Read 1 sector
    mov ch, BYTE [absoluteTrack]
    mov cl, BYTE [absoluteSector]
    mov dh, BYTE [absoluteHead]
    mov dl, BYTE [bsDriveNumber]
    int 0x13
    jnc .Success    ; Check for read error
    xor ax, ax      ; Reset disk
    int 0x13
    dec di
    pop cx
    pop bx
    pop ax
    jnz .SectorLoop ; Retry in case of an error
    int 0x18
  .Success:
    pop cx
    pop bx
    pop ax
    add bx, WORD [bpbBytesPerSector] ; Next buffer
    inc ax                           ; Next sector
    loop .Main                       ; Read next sector
    ret

; ======================
; Bootloader entry point
; ======================

Main:
  ; -------------------------------------
  ; Adjust segment registers to 0000:7C00
  ; -------------------------------------

  cli
  mov ax, 0x07c0
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax

  ; ------------
  ; Create stack
  ; ------------

  mov ax, 0x0000
  mov ss, ax
  mov sp, 0xffff
  sti

  ; --------------------
  ; Show loading message
  ; --------------------

  mov ax, 0x03 ; Clear screen
  int 0x10
  mov si, msgLoading
  call Puts16

  ; -------------------------
  ; Load root directory table
  ; -------------------------

  .LoadRoot:
    ; Compute root directory size => CX
    xor  cx, cx
    xor  dx, dx
    mov  ax, 32                   ; 32 B directory entry
    mul  WORD [bpbRootEntries]    ; Directory size in B
    div  WORD [bpbBytesPerSector] ; Sectors used by directory
    xchg ax, cx
    ; Compute root directory location => AX
    mov  al, BYTE [bpbNumberOfFATs]    ; Number of FATs
    mul  WORD [bpbSectorsPerFAT]       ; Sectors used by FATs
    add  ax, WORD [bpbReservedSectors] ; Adjust for bootsector
    mov  WORD [dataSector], ax         ; Start of root directory
    add  WORD [dataSector], cx
    ; Load root directory at 7C00:0200
    mov  bx, 0x0200
    call ReadSectors

  ; ----------------------------
  ; Find second stage bootloader
  ; ----------------------------

  ; Search root directory for binary image
  mov cx, WORD [bpbRootEntries] ; Directory size
  mov di, 0x0200                ; First entry
  .Loop:
    push cx
    mov  cx, 0x000B    ; 11 character name
    mov  si, imageName ; File name to search for
    push di
    rep cmpsb         ; Test for match
    pop  di
    je   .LoadFAT
    pop  cx
    add  di, 0x0020    ; Queue next entry
    loop .Loop
    jmp  Failure

  ; --------
  ; Load FAT
  ; --------

  .LoadFAT:
    ; Store first cluster of boot image
    mov dx, WORD [di + 0x001a]
    mov WORD [cluster], dx ; First cluster
    ; Compute FAT size => CX
    xor ax, ax
    mov al, BYTE [bpbNumberOfFATs]
    mul WORD [bpbSectorsPerFAT]
    mov cx, ax
    ; Compute FAT location => AX
    mov ax, WORD [bpbReservedSectors] ; Adjust for bootsector
    ; Load FAT at 7C00:0200
    mov bx, 0x0200
    call ReadSectors
    ; Load image file at 0050:0000
    mov ax, 0x0050
    mov es, ax
    mov bx, 0x0000
    push bx

  ; ----------------------------
  ; Load second stage bootloader
  ; ----------------------------

  .LoadImage:
    mov  ax, WORD [cluster] ; Cluster to read
    pop  bx                 ; Buffer to read to
    call CHSToLBA
    xor  cx, cx
    mov  cl, BYTE [bpbSectorsPerCluster] ; Sectors to read
    call ReadSectors
    push bx
    ; Compute next cluster
    mov ax, WORD [cluster] ; Current cluster
    mov cx, ax             ; Copy current cluster
    mov dx, ax             ; Copy current cluster
    shr dx, 0x0001         ; Divide by 2
    add cx, dx             ; Sum for 3/2
    mov bx, 0x0200         ; FAT location
    add bx, cx             ; FAT index
    mov dx, WORD [bx]      ; Read 2 bytes
    test ax, 0x0001
    jnz .OddCluster
  .EvenCluster:
    and dx, 0x0fff ; Low 12 bits
    jmp .Done
  .OddCluster:
    shr dx, 0x0004 ; High 12 bits
  .Done:
    mov WORD [cluster], dx ; Store new cluster
    cmp dx, 0x0FF0         ; Check for EOF
    jb .LoadImage

; ===============================
; Jump to second stage bootloader
; ===============================

Done:
  mov si, msgCRLF
  call Puts16
  push WORD 0x0050
  push WORD 0x0000
  retf

; =====================================
; Abort since a critical error occurred
; =====================================

Failure:
  mov ax, 0x03 ; Clear screen
  int 0x10
  mov si, msgFailure
  call Puts16
  mov ah, 0x01
  mov ch, 0x20
  mov cl, 0x20
  int 0x10 ; Disable cursor
  mov ah, 0x00
  int 0x16 ; Await keypress
  int 0x19 ; Warm boot computer

imageName   DB "BOOTMGR SYS"
msgLoading  DB "Loading boot image...", 0x00
msgCRLF     DB 0x0d, 0x0a, 0x00
msgFailure  DB "FATAL: BOOTMGR.SYS IS MISSING OR CURRUPT", 0x0d, 0x0a, \
               "Press any key to reboot", 0x00

TIMES 510-($-$$) DB 0
DW 0xaa55
