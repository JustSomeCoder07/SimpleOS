%ifndef FLOPPY16_INC
%define FLOPPY16_INC

bits 16

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

dataSector  DW 0x0000
cluster     DW 0x0000
absoluteSector DB 0x00
absoluteHead   DB 0x00
absoluteTrack  DB 0x00

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

%endif
