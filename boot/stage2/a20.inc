%ifndef A20_INC
%define A20_INC

bits 16

; ===================================
; Checks the A20 state.
;
; AX = 0 if the A20 Gate is enabled,
;      1 if the A20 Gate is disabled.
; ===================================

bufferBelowMB	DB 0
bufferOverMB	DB 0

GetA20State:

  ; Serialize registers, disable interrupts
  pushf
  push si
  push di
  push ds
  push es
  cli
  ; Set ds:si to 0x0008:0xe000 (0x0000E800)
  mov ax, 0x0000
  mov ds, ax
  mov si, 0xE800
  ; Set es:di to 0x0018:0xe000 (0x0010E800)
  mov ax, 0x0010
  mov es, ax
  mov di, 0xE800
  ; Save old values at the given memory addresses
  mov al, [ds:si]
  mov byte [bufferBelowMB], al
  mov al, [es:di]
  mov byte [bufferOverMB], al
  ; Check for memory wrapping (if the memory wraps at 1 MiB,
  ; then the A20 gate is enabled)
  mov ah, 1           ; Return value 1: A20 gate disabled
  mov byte [ds:si], 0 ; Set 0x0000E800                  to 0
  mov byte [es:di], 1 ; Set 0x0010E800 (and 0x0010E800) to 1
  mov al, [ds:si]     ; Set 0x0000E800                  to 0 again
  cmp al, [es:di]     ; Compare 0x0000E800 and 0x0010E800:
  jne .Exit           ; If the memory wraps around, the addresses are both 1,
                      ; If it doesn't, 0x0000E800 is 0, but 0x0010E800 is 1
  dec ah              ; Return value 0: A20 gate enabled

.Exit:

  ; Restore values at the old addresses
	mov al, [bufferBelowMB]
	mov [ds:si], al
	mov al, [bufferOverMB]
	mov [es:di], al
	shr ax, 8 ; Shift return code to correct position
  ; Restore registers & interrupts
	sti
	pop es
	pop ds
	pop di
	pop si
	popf

; ====================================================
; Checks support for the A20 line using BIOS routines.
;
; CF: Clear if the A20 line is supported.
; AX: 0 if the A20 line is not supported.
;     1 if the A20 line is supported.
; ====================================================

QueryA20Support:
	push bx
	clc
	mov ax, 0x2403
	int 0x15       ; Query if the A20 line is supported
	jc .Error      ; Not supported
	test ah, ah
	jnz .Error
	mov ax, bx
	pop bx
	ret
.Error:
	stc
	pop bx
	ret

; ==================================================
; Enables the A20 line via the 8042 PS/2 controller.
; ==================================================

EnableA20_KbCtrl:
	cli

  ; Disable keyboard
	call .WaitIO1
	mov  al, 0xAD
	out  0x64, al
  ; Read from controller output port
	call .WaitIO1
	mov  al, 0xD0
	out  0x64, al
  ; Get status byte
	call .WaitIO2
	in   al, 0x60
	push ax
  ; Write next byte to output
	call .WaitIO1
	mov  al, 0xD1
	out  0x64, al
  ; Bit 1: Enable A20 line
	call .WaitIO1
	pop   ax
	or   al, 2
	out  0x60, al
  ; Re-enable keyboard
	call .WaitIO1
	mov  al, 0xae
	out  0x64, al
	call .WaitIO2
	sti
	ret

; ==================================================
; Waiting routines
;
; Both routines test (for different values) against
; the port 0x64.
; As some ways to enable the A20 line are very slow,
; testing of the A20 line status is done in a loop.
; ==================================================

.WaitIO1:
	in   al, 0x64
	test al, 2
	jnz  .WaitIO1
	ret

.WaitIO2:
	in   al, 0x64
	test al, 1
	jz   .WaitIO2
	ret

; ===========================================
; Enables the A20 line
;
; CF is cleared if the A20 line is supported.
; ===========================================

EnableA20:
	clc
	pusha
	mov   bh, 0

  ; ------------------------------------
  ; Check if A20 gate is already enabled
  ; ------------------------------------

	call GetA20State
	jc   .EnableA20_FastGate

	test ax, ax
	jnz  .Done

	call QueryA20Support
	mov  bl, al
	test bl, 1
	jnz  .KbCtrl

	test bl, 2
	jnz  .EnableA20_FastGate

  ; --------------------------
  ; Enable A20 line using BIOS
  ; --------------------------

  .EnableA20_BIOS:

    mov  ax, 0x2401
    int  0x15
    jc   .EnableA20_FastGate
    test ah, ah
    jnz  .Failed
    call GetA20State
    test ax, ax
    jnz  .Done

  ; -------------------------------
  ; Enable A20 line using Fast Gate
  ; -------------------------------

  .EnableA20_FastGate:

    in   al, 0x92
    test al, 2
    jnz  .Done

    or  al, 2
    and al, 0xfe
    out 0x92, al

    call GetA20State
    test ax, ax
    jnz  .Done
    test bh, bh
    jnz  .Failed

  ; -----------------------------------------
  ; Enable A20 line using keyboard controller
  ; -----------------------------------------

  .KbCtrl:
    call EnableA20_KbCtrl
    call GetA20State
    test ax, ax
    jnz  .Done
    ; Try to enable A20 line using keyboard controller flags
    mov  bh, 1
    test bl, 2
    jnz  .EnableA20_FastGate
    jmp  .Failed

  ; ---------------------------------
  ; The A20 line could not be enabled
  ; ---------------------------------

  .Failed:
    stc

  ; -----------------------------
  ; The A20 line has been enabled
  ; -----------------------------

  .Done:
    popa
    ret

%endif
