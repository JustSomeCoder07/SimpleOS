%ifndef _COMMON_INC
%define _COMMON_INC

; Kernel loading address
%define IMAGE_PMODE_BASE 0x100000 ; Protected Mode
%define IMAGE_RMODE_BASE 0x3000   ; Real Mode

imageName DB "KERNEL  SYS" ; Kernel name
imageSize DB 0 ; Kernel size in bytes

%endif
