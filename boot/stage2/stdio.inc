%ifndef STDIO_INC
%define STDIO_INC

; ***************
; 16-bit routines
; ***************

bits 16

; ==========================
; Prints a string
;
; SI: Null-terminated string
; ==========================

Puts16:
  pusha
  .Loop:
    lodsb
    or    al, al
    jz    .Done ; Check for null terminator
    mov   ah, 0x0e
    int   0x10
    jmp   .Loop
  .Done:
    popa
    ret

; ***************
; 32-bit routines
; ***************

bits 32

%define VID_MEM   0xB8000
%define VID_COLS  80
%define VID_ROWS  25
%define CHAR_ATTR 63 ; FG: white, BG: light blue

; Cursor position
CurX DB 0
CurY DB 0

; ======================
; Prints a character
;
; BL: Character to print
; ======================

Putch32:
  pusha

  ; --------------------
  ; Get current position
  ;
  ; x + y * COLS
  ; --------------------

  mov  edi, VID_MEM
  xor  eax, eax
  mov  ecx, VID_COLS * 2 ; 2 bytes per char
  mov  al, BYTE [CurY]
  mul  ecx
  push eax
  mov  al, BYTE [CurX]
  mov  cl, 2             ; 2 bytes per char
  mul  cl
  pop  ecx
  add  eax, ecx
  xor  ecx, ecx
  add  edi, eax

  ; -----------------
  ; Check for newline
  ; -----------------

  cmp bl, 0x0A ; Newline character
  je  .Row

  ; -----------------
  ; Print a character
  ; -----------------

  mov dl, bh        ; Character
  mov dh, CHAR_ATTR ; Character attribute
  mov WORD [edi], dx

  ; ---------------
  ; Update position
  ; ---------------

  inc BYTE [CurX]
  cmp BYTE [CurX], VID_COLS ; Check for end of line
  je  .Row
  jmp .Done

  ; --------------
  ; Go to next row
  ; --------------

  .Row:
    mov BYTE [CurX], 0
    inc BYTE [CurY]

  .Done:
    popa
    ret

; ===========================
; Prints a string
;
; EBX: Null-terminated string
; ===========================

Puts32:
  pusha
  push  ebx ; Store string address
  pop   edi

  ; ------------
  ; Print string
  ; ------------
  .Loop:
    mov  bl, BYTE [edi] ; Character
    cmp  bl, 0          ; Check if character is null terminator
    je   .Done
    call Putch32
    inc  edi
    jmp .Loop

  ; ----------------------
  ; Update hardware cursor
  ; ----------------------
  .Done:
    mov  bh, BYTE [CurY]
    mov  bl, BYTE [CurX]
    call MovCur
    popa
    ret

; ---------------------------
; Updates the hardware cursor
;
; BH: Y position
; BL: X position
; ---------------------------

MovCur:
  pusha

  ; --------------------
  ; Get current position
  ;
  ; x + y * COLS
  ; --------------------

  xor eax, eax
  mov al, bh
  mul ecx
  add al, bl
  mov ebx, eax

  ; ------------------------------------
  ; Write low byte index to VGA register
  ; ------------------------------------

  mov al, 0x0f
  mov dx, 0x03d4
  out dx, al
  mov al, bl
  mov dx, 0x03d5
  out dx, al

  ; -------------------------------------
  ; Write high byte index to VGA register
  ; -------------------------------------

  xor eax, eax
  mov al, 0x0e
  mov dx, 0x03d4
  out dx, al
  mov al, bl
  mov dx, 0x03d5
  out dx, al

  popa
  ret

; =================
; Clears the screen
; =================

Cls32:
  pusha
  cld
  mov   edi, VID_MEM
  mov   cx, 80 * 50
  mov   ah, CHAR_ATTR
  mov   al, ' '
  rep stosw
  mov   BYTE [CurX], 0
  mov   BYTE [CurY], 0
  popa
  ret

; =====================
; Sets current position
;
; AL: X position
; AH: Y position
; =====================

SetPos:
  pusha
  mov   [CurX], al
  mov   [CurY], ah
  popa
  ret

%endif
