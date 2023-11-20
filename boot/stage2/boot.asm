bits 16
org 0x500 ; Loaded at 0050:0000

start: jmp Main

%include "stdio.inc"  ; Routines for I/O
%include "gdt.inc"    ; Routines to setup GDT
%include "a20.inc"    ; Routines to enable A20 line
%include "fat12.inc"  ; Primitive FAT12 driver
%include "common.inc" ; Common data

msgLoading     DB "Searching for operating system...", 0x0D, 0x0A, 0x00
msgFailure     DB "FATAL: KERNEL.SYS IS MISSING OR CORRUPT", 0x0D, 0x0A, \
                  "Press any key to reboot", 0x00
msgA20Failure: DB "FATAL: FAILED TO ENABLE A20 LINE", 0x0D, 0x0A, \
                  "Press any key to reboot", 0x00
msgA20Success: DB "A20 Gate disabled", 0x0D, 0x0A, 0x00

; =============================================================== ;
; A20 line failure
;
; This routine is called when the A20 Gate couldn't be disabled,
; making it impossible to use more than 1 MiB of memory.
; Running the kernel under these restrictions would be pointless,
; so the code only displays an error message and waits for user
; interaction to reboot.
; =============================================================== ;

A20Failure:
    mov ax, 0x03 ; Clear screen
    int 0x10
    mov ah, 0x01
    mov ch, 0x20
    mov cl, 0x20
    int 0x10              ; Disable cursor
    mov si, msgA20Failure
    call Puts16
    mov ah, 0x00
    int 0x16              ; Wait for keypress
    mov ah, 0x00
    int 0x19              ; Warmboot computer
    cli
    hlt

; ***********************************
; Second Stage Bootloader entry point
; ***********************************

Main:
  ; Setup segments and stack
  cli
  xor ax, ax
  mov ds, ax
  mov es, ax
  mov ax, 0x00
  mov ss, ax
  mov ss, ax
  mov sp, 0xffff
  sti

  call InstallGDT ; Install GDT

  ; Enable A20 line
  call EnableA20;
  jc  A20Failure;
  mov  si, msgA20Success
  call Puts16

  ; Display loading message
  mov  si, msgLoading
  call Puts16

  ; Initialize filesystem
  call LoadRoot

  ; Load kernel
  mov  ebx, 0                 ; BX:BP: Buffer to load to
  mov  bp, IMAGE_RMODE_BASE
  mov  si, imageName          ; File to load
  call LoadFile
  mov  DWORD [imageSize], ecx ; Store kernel size
  cmp  ax, 0
  je   PrepareStage3          ; If successful, go to Stage 3
  mov ax, 0x03 ; Clear screen
  int 0x10
  mov  si, msgFailure
  call Puts16
  mov  ah, 0x01
  mov  ch, 0x20
  mov  cl, 0x20
  int  0x10                   ; Disable cursor
  mov  ah, 0
  int  0x16                   ; Wait for keypress
  int  0x19                   ; Warmboot computer

; ===============================
; Switch to 32-bit Protected Mode
; ===============================

PrepareStage3:
  cli

  ; Set bit 0 in cr0
  mov eax, cr0
  or eax, 1
  mov cr0, eax

  jmp CODE_DESC:Stage3

; **********************************
; Third Stage Bootloader entry point
; **********************************

bits 32

Stage3:
  ; Set registers
  mov ax, DATA_DESC
  mov ds, ax
  mov ss, ax
  mov es, ax
  mov esp, 0x90000

; =====================
; Loads kernel at 1 MiB
; =====================

CopyImage:
  mov   eax, DWORD [imageSize]
  movzx ebx, WORD [bpbBytesPerSector]
  mul ebx
  mov ebx, 4
  div ebx
  cld
  mov esi, IMAGE_RMODE_BASE
  mov edi, IMAGE_PMODE_BASE
  mov ecx, eax
  rep movsd ; Copy image to destination address

; ===============
; Executes kernel
; ===============
StartKernel:
  jmp CODE_DESC:IMAGE_PMODE_BASE
  ; Stop execution
  cli
  hlt
