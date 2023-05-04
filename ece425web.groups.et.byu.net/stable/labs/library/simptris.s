	align	2
SlidePiece:	; void SlidePiece(int ID, int direction)
	push	bp
	mov	bp, sp
	push	ax
	push	dx
 	mov	ah, 0		; Slide piece operation
  	mov	al, [bp+6]	; Move direction (loword only)
	mov	dx, [bp+4]	; Move id
	int	1Bh		; Call Simptris services
	pop	dx
	pop	ax
	pop	bp
	ret
RotatePiece:	; void RotatePiece(int ID, int direction)
	push	bp
	mov	bp, sp
	push	ax
	push	dx
 	mov	ah, 1		; Rotate piece operation
 	mov	al, [bp+6]	; Move direction (loword only)
	mov	dx, [bp+4]	; Move id
	int	1Bh		; Call Simptris services
	pop	dx
	pop	ax
	pop	bp
	ret
SeedSimptris:	; void SeedSimptris(long seed)
	push	bp
	mov	bp, sp
	push	ax
	push	cx
	push    dx
 	mov	ah, 2		; Seed operation
	mov	dx, [bp+4]	; Move loword of seed
	mov	cx, [bp+6]	; Move hiword of seed
	int	1Bh		; Call Simptris services
	pop	dx
	pop	cx
	pop	ax
	pop	bp
	ret
StartSimptris:	; void StartSimptris(void)
	push	ax
 	mov	ah, 3		; Start Simptris operation
	int	1Bh		; Call Simptris services
	pop	ax
	ret

