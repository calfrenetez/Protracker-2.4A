#NO_APP
	.text
	.align	2
	.globl	_repro
_repro:
	link.w a5,#0
	move.l a2,-(sp)
	move.l (8,a5),a2
	move.l (a2),d0
	addq.l #1,d0
	move.l d0,(a2)
	cmp.l (4,a2),d0
	jeq .L3
	move.l (12,a5),-(sp)
	jsr _malloc
	addq.l #4,sp
	tst.l d0
	jeq .L1
	addq.l #1,(8,a2)
	jra .L1
.L3:
	moveq #0,d0
.L1:
	move.l (-4,a5),a2
	unlk a5
	rts
