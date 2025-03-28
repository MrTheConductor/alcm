 ;
 ; Copyright (c) 2024-2025, Mitchell White <mitchell.n.white@gmail.com>
 ;
 ; This file is part of Advanced LCM (ALCM) project.
 ; 
 ; ALCM is free software: you can redistribute it and/or modify it under the
 ; terms of the GNU General Public License as published by the Free Software
 ; Foundation, either version 3 of the License, or (at your option) any later
 ; version.
 ; 
 ; ALCM is distributed in the hope that it will be useful, but WITHOUT ANY
 ; WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 ; FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 ; details.
 ; 
 ; You should have received a copy of the GNU General Public License along
 ; with ALCM. If not, see <https://www.gnu.org/licenses/>.
 ;
    AREA WS2812, CODE, READONLY
    EXPORT ws2812_send_buffer

T0H EQU 1
T1H EQU 3
TRESET EQU 400

PIN_SET EQU (1 << 4)
PIN_RESET EQU (1 << (4 + 16))
GPIOD_BSRR EQU 0x48000C18


; This function is used to send WS2812 buffer. It is written in
; assembly because of the tight timing requirements of the WS2812
; protocol.
;
; This prevents compiler optimization changes from causing timing
; issues by carrying out the timing critical code in assembly.
;
; R0 = uint8_t* buffer
; R1 = uint32_t length
;
; Returns nothing
ws2812_send_buffer
    PUSH {R4-R7, LR}

    CMP R0, #0                    ; Check if buffer is NULL
    BEQ end_function              ; If it is, return

    CMP R1, #0                    ; Check if length is 0
    BEQ end_function              ; If it is, return

    ; Pre-load constants
    LDR R6, =GPIOD_BSRR

next_byte
    LDRB R2, [R0]
    ADDS R0, R0, #1

    ; Shift up to the MSB
    LSLS R2, R2, #24

    ; Setup 8-bit loop counter
    MOVS R3, #8

next_bit
    LSLS R2, R2, #1
    BCC bit_0
    B bit_1

bit_0
	MOVS R4, #T0H
    B send_bit

bit_1
	MOVS R4, #T1H
    B send_bit

send_bit
	MOVS R7, #PIN_SET
    STR R7, [R6]

high_delay
    SUBS R4, R4, #1
    BNE high_delay

    LDR R7, =PIN_RESET
    STR R7, [R6]

    ; Next bit in byte
    SUBS R3, R3, #1
    BNE next_bit

    ; Next byte in buffer
    SUBS R1, R1, #1
    BNE next_byte

    ; 50 us delay required for Treset
	LDR R4, =TRESET
t_reset
    SUBS R4, R4, #1
    BNE t_reset

end_function
	NOP
    POP {R4-R7, PC}
    END