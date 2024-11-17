; Draw background
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


; Draw cube
mov bp, 64*4*10 + 4*8 ; Screen buffer top left of cube
mov dx, 0      ; Initialize y coordinate
mov cx, 0      ; Initialize x coordinate

; Set color to bright blue (50, 50, 255, 255)
mov al, 50     ; Store color values in registers
mov ah, 255    ; for reuse

; Draw front face vertices and edges
mov cx, 32     ; Line length

; Top horizontal line
draw_back_top:
    mov [bp], byte 255   ; Red
    mov [bp+1], byte 255 ; Green
    mov [bp+2], byte 255 ; Blue
    mov [bp+3], byte 255 ; Alpha
    add bp, 4
    sub cx, 1
    jnz draw_back_top

; Right vertical line
mov cx, 32
draw_back_right:
    mov [bp], byte 255
    mov [bp+1], byte 255
    mov [bp+2], byte 255
    mov [bp+3], byte 255
    add bp, 256   ; Move down one row (64*4)
    sub cx, 1
    jnz draw_back_right

; Bottom horizontal line
mov cx, 32
draw_back_bottom:
    mov [bp], byte 255
    mov [bp+1], byte 255
    mov [bp+2], byte 255
    mov [bp+3], byte 255
    sub bp, 4     ; Move left
    sub cx, 1
    jnz draw_back_bottom

; Left vertical line (completing front face)
mov cx, 32
draw_back_left:
    mov [bp], byte 255
    mov [bp+1], byte 255
    mov [bp+2], byte 255
    mov [bp+3], byte 255
    sub bp, 256   ; Move up one row
    sub cx, 1
    jnz draw_back_left


mov bp, 64*4*20 + 4*18  ; Screen buffer starts at 0x00f0
mov cx, 32
draw_front_top:
    mov [bp], byte 255   ; Red
    mov [bp+1], byte 255 ; Green
    mov [bp+2], byte 255 ; Blue
    mov [bp+3], byte 255 ; Alpha
    add bp, 4
    sub cx, 1
    jnz draw_front_top

mov cx, 32
draw_front_right:
    mov [bp], byte 255
    mov [bp+1], byte 255
    mov [bp+2], byte 255
    mov [bp+3], byte 255
    add bp, 256
    sub cx, 1
    jnz draw_front_right

mov cx, 32
draw_front_bottom:
    mov [bp], byte 255
    mov [bp+1], byte 255
    mov [bp+2], byte 255
    mov [bp+3], byte 255
    sub bp, 4     ; Move left
    sub cx, 1
    jnz draw_front_bottom

mov cx, 32
draw_front_left:
    mov [bp], byte 255
    mov [bp+1], byte 255
    mov [bp+2], byte 255
    mov [bp+3], byte 255
    sub bp, 256
    sub cx, 1
    jnz draw_front_left



; Draw the four diagonal lines for depth
mov cx, 10    ; Depth of the cube

; Top-left diagonal
mov dx, cx   ; Store counter
mov bp, 64*4*10 + 4*8
top_left_diagonal:
    add bp, 260   ; Move down-right (64*4 + 4)
    mov [bp], byte 255
    mov [bp+1], byte 255
    mov [bp+2], byte 255
    mov [bp+3], byte 255
    sub dx, 1
    jnz top_left_diagonal


; Top-right diagonal
mov dx, cx   ; Store counter
mov bp, 64*4*10 + 4*40
top_right_diagonal:
    add bp, 260   ; Move down-right (64*4 + 4)
    mov [bp], byte 255
    mov [bp+1], byte 255
    mov [bp+2], byte 255
    mov [bp+3], byte 255
    sub dx, 1
    jnz top_right_diagonal

; Bottom-left diagonal
mov dx, cx   ; Store counter
mov bp, 64*4*42 + 4*8
bot_left_diagonal:
    add bp, 260   ; Move down-right (64*4 + 4)
    mov [bp], byte 255
    mov [bp+1], byte 255
    mov [bp+2], byte 255
    mov [bp+3], byte 255
    sub dx, 1
    jnz bot_left_diagonal

; Bottom-left diagonal
mov dx, cx   ; Store counter
mov bp, 64*4*42 + 4*40
bot_right_diagonal:
    add bp, 260   ; Move down-right (64*4 + 4)
    mov [bp], byte 255
    mov [bp+1], byte 255
    mov [bp+2], byte 255
    mov [bp+3], byte 255
    sub dx, 1
    jnz bot_right_diagonal
