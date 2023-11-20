%ifndef GDT_INC
%define GDT_INC

bits 16

; ================
; Installs the GDT
; ================

InstallGDT:
  cli
  pusha
  lgdt [TOC] ; Load GDT into GDTR
  sti
  popa
  ret

; =============================
; Global Descriptor Table (GDT)
; =============================

%define NULL_DESC 0x00
%define CODE_DESC 0x08
%define DATA_DESC 0x10

GDTData:
  ; ---------------
  ; Null descriptor
  ; ---------------

  DD 0
  DD 0

  ; ---------------
  ; Code descriptor
  ; ---------------

  DW 0xFFFF    ; Limit low
  DW 0         ; Base low
  DB 0         ; Base middle
  DB 10011010b ; Access
  DB 11001111b ; Granularity
  DB 0         ; Base high

  ; ---------------
  ; Data descriptor
  ; ---------------

  DW 0xFFFF    ; Limit low
  DW 0         ; Base low
  DB 0         ; Base middle
  DB 10010010b ; Access
  DB 11001111b ; Granularity
  DB 0         ; Base high

  EndOfGDT:
  TOC:
    DW EndOfGDT - GDTData - 1 ; Limit of GDT
    DD GDTData                ; Base  of GDT

%endif
