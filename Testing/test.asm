bits 16

mov bp, 0x00f0 ; location of the pixel buffer
mov dx, 0
y_loop_start:
	
	mov cx, 0
	x_loop_start:
		mov ax, dx
		mov bx, cx
		add ax, 50
		add bx, 50

		; Fill pixel
		mov byte [bp + 0], al ; Red
		mov byte [bp + 1], 0 ; green
		mov byte [bp + 2], bl ; Blue
		mov byte [bp + 3], 255 ; Alpha

		; Advance pixel location
		add bp, 4
			
		; Advance X coordinate and loop
		add cx, 1
		cmp cx, 64
		jnz x_loop_start
	
	; Advance Y coordinate and loop
	add dx, 1
	cmp dx, 64
	jnz y_loop_start