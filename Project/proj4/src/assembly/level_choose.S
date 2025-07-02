.section .text
.globl CreateLevelChoose
.type CreateLevelChoose, @function
CreateLevelChoose:
	li a0,60
	li a1,15
	la a2, Level_1
	li a3, 0xFFFF
	addi sp,sp,-4
	sw ra,0(sp)
	jal LCD_ShowString
	lw ra,0(sp)
	addi sp,sp,4
	li a0,60
	li a1,35
	la a2, Level_2
	li a3, 0xFFFF
	addi sp,sp,-4
	sw ra,0(sp)
	jal LCD_ShowString
	lw ra,0(sp)
	addi sp,sp,4
	li a0,60
	li a1,55
	la a2, Level_3
	li a3, 0xFFFF
	addi sp,sp,-4
	sw ra,0(sp)
	jal LCD_ShowString
	lw ra,0(sp)
	addi sp,sp,4
.globl Level_Update
.type Level_Update, @function
Level_Update:
	la t1,Current_selection
	lw t1,0(t1)
	li t2,3
	rem t1,t1,t2
	addi t2,x0,0
	beq t1,t2,Level_1_Update
	addi t2,x0,1
	beq t1,t2,Level_2_Update
	addi t2,x0,2
Level_3_Update:
	li a0,60
	li a1,15
	la a2, Level_1
	li a3, 0xFFFF
	addi sp,sp,-4
	sw ra,0(sp)
	jal LCD_ShowString
	lw ra,0(sp)
	addi sp,sp,4
	li a0,60
	li a1,35
	la a2, Level_2
	li a3, 0xFFFF
	addi sp,sp,-4
	sw ra,0(sp)
	jal LCD_ShowString
	lw ra,0(sp)
	addi sp,sp,4
	li a0,60
	li a1,55
	la a2, Level_3
	li a3, 0x001F
	addi sp,sp,-4
	sw ra,0(sp)
	jal LCD_ShowString
	lw ra,0(sp)
	addi sp,sp,4
	ret
Level_2_Update:
	li a0,60
	li a1,15
	la a2, Level_1
	li a3, 0xFFFF
	addi sp,sp,-4
	sw ra,0(sp)
	jal LCD_ShowString
	lw ra,0(sp)
	addi sp,sp,4
	li a0,60
	li a1,35
	la a2, Level_2
	li a3, 0x001F
	addi sp,sp,-4
	sw ra,0(sp)
	jal LCD_ShowString
	lw ra,0(sp)
	addi sp,sp,4
	li a0,60
	li a1,55
	la a2, Level_3
	li a3, 0xFFFF
	addi sp,sp,-4
	sw ra,0(sp)
	jal LCD_ShowString
	lw ra,0(sp)
	addi sp,sp,4
	ret
Level_1_Update:
	li a0,60
	li a1,15
	la a2, Level_1
	li a3, 0x001F
	addi sp,sp,-4
	sw ra,0(sp)
	jal LCD_ShowString
	lw ra,0(sp)
	addi sp,sp,4
	li a0,60
	li a1,35
	la a2, Level_2
	li a3, 0xFFFF
	addi sp,sp,-4
	sw ra,0(sp)
	jal LCD_ShowString
	lw ra,0(sp)
	addi sp,sp,4
	li a0,60
	li a1,55
	la a2, Level_3
	li a3, 0xFFFF
	addi sp,sp,-4
	sw ra,0(sp)
	jal LCD_ShowString
	lw ra,0(sp)
	addi sp,sp,4
	ret
.globl Press_Button
.type Press_Button, @function
Press_Button:
	mv t1,a0
	li t2,3
	rem t1,t1,t2
	addi t1,t1,1
	mv a0,t1
	ret
.globl Selecting_ASM
.type Selecting_ASM, @function

Selecting_ASM:
    addi sp, sp, -16
    sw ra, 12(sp)
    sw s0, 8(sp)
    sw s1, 4(sp)
    sw s2, 0(sp)
    
main_loop:
    jal Level_Update
    
    jal Update_Button
    
check_joy_down:
    li a0, 1                    # JOY_DOWN
    jal Get_Button
    beqz a0, check_joy_up    
    
    li a0, 1                    # JOY_DOWN_BTN
    jal Query_Button_locked
    bnez a0, check_joy_up     
    

    la s0, Current_selection
    lw s1, 0(s0)
    addi s1, s1, 1
    
    li t0, 5
    ble s1, t0, save_down
    li s1, 3
    
save_down:
    sw s1, 0(s0)         
    
    li a0, 1
    jal Lock_Button
    
check_joy_up:
    li a0, 3                    # JOY_UP
    jal Get_Button
    beqz a0, check_joy_ctr
    
    li a0, 0                    # JOY_UP_BTN
    jal Query_Button_locked
    bnez a0, check_joy_ctr
    
    la s0, Current_selection
    lw s1, 0(s0)
    addi s1, s1, -1
    
    li t0, 1
    bge s1, t0, save_up
    li s1, 3
    
save_up:
    sw s1, 0(s0)        
    
    li a0, 0
    jal Lock_Button
    
check_joy_ctr:
    li a0, 4                    # JOY_CTR
    jal Get_Button
    beqz a0, delay_and_loop
    
    li a0, 4                    # JOY_CTR_BTN
    jal Query_Button_locked
    bnez a0, delay_and_loop
    
    li a0, 0                    # BLACK
    jal LCD_Clear
    
    la t0, Current_selection
    lw a0, 0(t0)
    jal Press_Button
    mv s2, a0            
    
    li a0, 4
    jal Lock_Button
    
    mv a0, s2
    j exit_function
    
delay_and_loop:
    li a0, 20
    jal delay_1mss
    j main_loop
    
exit_function:

    lw s2, 0(sp)
    lw s1, 4(sp)
    lw s0, 8(sp)
    lw ra, 12(sp)
    addi sp, sp, 16
    
    ret



delay_1mss:
    addi sp, sp, -16
    sw ra, 12(sp)
    sw s0, 8(sp)
    sw s1, 4(sp)
    sw s2, 0(sp)
    mv s0, a0
    la t0, SystemCoreClock
    lw t1, 0(t0)
    li t2, 4000
    divu t1, t1, t2
    mul s1, t1, s0
    jal get_low_timer
    mv s2, a0
    
Warming_up:
    jal get_low_timer
    beq a0, s2, Warming_up
    mv s2, a0
delay_loop:
    jal get_low_timer
    sub t0, a0, s2
    bltu t0, s1, delay_loop
    lw s2, 0(sp)
    lw s1, 4(sp)
    lw s0, 8(sp)
    lw ra, 12(sp)
    addi sp, sp, 16
    
    ret
#TIMER_CTRL_ADDR
get_low_timer:
	li a0, 0xd1000000
	lw a0, 0(a0)
	ret