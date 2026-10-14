* Local labels, MEMORY map, INCLUDE/INCBIN, long lines, tabs
	ORG	$1000
MAIN	MOVE.L	#10,D0
.loop	SUBQ.L	#1,D0
	BNE	.loop
SUB1	NOP
.loop	BRA.S	.loop
	MEMORY	ROM,$0,$FFF
	MEMORY	READ,$4000,$4FFF
	MEMORY	PROTECTED,$5000,$5FFF
	MEMORY	INVALID,$6000,$6FFF
	INCLUDE	'inc_sub.x68'
	INCBIN	'inc_data.bin'
	DC.W	SUBVAL
	MOVE.L	#$12345678,D0	; a comment with	tabs
	MOVE.B	D0,D1 * not a comment marker in operand field
	DC.B	1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50
	SIMHALT
	END	MAIN
*~Font name~Courier New~
*~Font size~10~
*~Tab type~0~
*~Tab size~8~
