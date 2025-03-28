# TODO: This is lab1.1
/* Real Mode Hello World */
.code16

.global start
start:
	movw %cs, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %ss
	movw $0x7d00, %ax
	movw %ax, %sp # setting stack pointer to 0x7d00
	# TODO:通过中断输出Hello World, 并且间隔1000ms打印新的一行
	
	cli
	lea	clock_handle, %ax
	movw	%ax, 0x20
	movw	%cs, 0x22	

    # 发送命令字节到控制寄存器端口0x43
  movw $0x36, %ax         #方式3 ， 用于定时产生中断00110110b
  movw $0x43, %dx
  out %al, %dx 
           # 计算计数值， 产生20 毫秒的时钟中断， 时钟频率为1193180 赫兹
           # 计数值 = ( 时钟频率/ 每秒中断次数) − 1
           #       = (1193180 / (1 / 0.02 ) ) − 1= 23863
  movw $23863, %ax
          # 将计数值分为低字节和高字节， 发送到计数器0的数据端口（ 端口0x40 ）
  movw $0x40, %dx
  out %al, %dx 
  mov %ah, %al
  out %al, %dx
  sti


loop:
	jmp loop


# 处理中断的代码                
clock_handle:                    
# 20ms * 50 = 1s
	movw	counter, %ax
	incw	%ax
	movw	%ax, counter
	cmpw	$50, %ax
	jne skip_print

	# 达到1秒后复位计数器
	movw	$0, counter

	# 通过BIOS中断打印字符串
	lea	message, %si
print_loop:
	lodsb	
	cmpb	$0, %al		# 遇到字符串结束符则结束打印
	je print_done
	movb	$0x0E, %ah
	int	$0x10	
	jmp	print_loop
print_done:

skip_print:
	# 发送EOI给主PIC
	movb	$0x20, %al
	movw	$0x20, %dx
	outb	%al, %dx

    iret 


message:
	.string "Hello, World!\r\n\0"
counter:
	.word 0







