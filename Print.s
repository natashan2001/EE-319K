; Print.s
; Student names: Natasha Nehra and Siobhan Madden
; Last modification date: 4/19/20
; EE319K lab 7 device driver for any LCD
;
; As part of Lab 7, students need to implement these LCD_OutDec and LCD_OutFix
; This driver assumes two low-level LCD functions
; ST7735_OutChar   outputs a single 8-bit ASCII character
; ST7735_OutString outputs a null-terminated string 

    IMPORT   ST7735_OutChar
    IMPORT   ST7735_OutString
    EXPORT   LCD_OutDec
    EXPORT   LCD_OutFix

    AREA    |.text|, CODE, READONLY, ALIGN=2
		PRESERVE8
    THUMB
number EQU 0
decimal EQU 4
  
;-----------------------LCD_OutDec-----------------------
; Output a 32-bit number in unsigned decimal format
; Input: R0 (call by value) 32-bit unsigned number
; Output: none
; Invariables: This function must not permanently modify registers R4 to R11
LCD_OutDec

	PUSH {LR, R0-R4}
	SUB SP, #8 ;allocation
	SUBS R1, R0, #10
	BMI ADD30
	MOV R2, #0xA
	UDIV R3, R0, R2
	MUL R2, R3, R2
	SUB R1, R0, R2
	MOV R0, R3
	STR R1, [SP, #number]
	BL LCD_OutDec
	LDR R0, [SP, #number]
	ADD R0, #0x30 
	BL ST7735_OutChar
RETURN
	ADD SP, #8 ;deallocation
	POP {LR, R0-R4}
    BX  LR
ADD30
	ADD R0, #0x30
	BL ST7735_OutChar
	B RETURN

;* * * * * * * * End of LCD_OutDec * * * * * * * *

; -----------------------LCD _OutFix----------------------
; Output characters to LCD display in fixed-point format
; unsigned decimal, resolution 0.01, range 0.00 to 9.99
; Inputs:  R0 is an unsigned 32-bit number
; Outputs: none
; E.g., R0=0,    then output "0.00 "
;       R0=3,    then output "0.03 "
;       R0=89,   then output "0.89 "
;       R0=123,  then output "1.23 "
;       R0=999,  then output "9.99 "
;       R0>999,  then output "*.** "
; Invariables: This function must not permanently modify registers R4 to R11
LCD_OutFix
	MOV R1, #0

AGAIN
	PUSH {LR, R0-R5}
	SUB SP, #8 ;allocation
	MOV R4, #1000
	SUBS R5, R0, R4
	BHS AST
	SUBS R5, R1, #3
	BEQ RET
	MOV R2, #0xA
	UDIV R3, R0, R2
	MUL R2, R3, R2
	SUB R2, R0, R2
	MOV R0, R3
	STR R2, [SP, #number]
	STR R1, [SP, #decimal]
	ADD R1, #1
	BL AGAIN
	LDR R0, [SP, #number]
	ADD R0, #0x30
	BL ST7735_OutChar
	LDR R1, [SP, #decimal]
	SUBS R1, R1, #2
	BNE RET
	MOV R0, #0x2E
	BL ST7735_OutChar
RET
	ADD SP, #8 ;deallocation
	POP {LR, R0-R5}
     BX   LR
AST
	MOV R0, #0x2A
	BL ST7735_OutChar
	MOV R0, #0x2E
	BL ST7735_OutChar
	MOV R0, #0x2A
	BL ST7735_OutChar
	MOV R0, #0x2A
	BL ST7735_OutChar
	MOV R0, #0x2A
	BL ST7735_OutChar
	B RET
	ALIGN
 
     
;* * * * * * * * End of LCD_OutFix * * * * * * * *

     ALIGN                           ; make sure the end of this section is aligned
     END                             ; end of file
