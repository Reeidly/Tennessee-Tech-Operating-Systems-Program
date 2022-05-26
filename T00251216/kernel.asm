GLOBAL k_print
GLOBAL lidtr
GLOBAL go
GLOBAL dispatch
GLOBAL outportb
GLOBAL init_timer_dev
EXTERN running
EXTERN enqueue_priority
EXTERN dequeue
EXTERN setRunning

k_print:
    push ebp
    mov ebp, esp
    pushad
    pushf
    
    
    mov ecx, 80

    mov eax, [ebp+16]
    mul ecx
    ;result of multiplication stored in edx:eax
    mov ebx, [ebp+20]
    add eax, ebx 
    mov ecx, 2
    mul ecx

    add eax, 0xB8000     ;this is the video memory location you're writing to
    
    mov ebx, [ebp+12]

    mov esi, [ebp+8]
    
    mov edi, eax
    loop:
        
        cmp ebx, 0
        je endLoop
        mov edx, 0xB8000+80*25*2
        cmp edi, edx
        jg endLoop

        ;this is when stuff gets wild
        ;increment to next space
        movsb 
        mov ecx, 0x8C
        mov [edi], cl 
        add edi, 1
        sub ebx, 1
        jmp loop

    endLoop:
    popf
    popad
    pop ebp
   
ret


lidtr:
    push ebp 
    mov ebp, esp 

    push eax 
    mov eax, [ebp+8]
    lidt [eax]
    pop eax

    pop ebp 
    ret

dispatch:
    pushad
    push ds
    push es
    push fs
    push gs

    push esp
    
    call setRunning
    pop esp
    mov eax, DWORD [running]

    push eax
    

    call enqueue_priority
    pop eax

    call dequeue
    mov DWORD [running], eax
    mov esp, DWORD [eax]

    pop gs
    pop fs
    pop es
    pop ds
    popad


    push eax
    ;send EOI to PIC
    mov al, 0x20
    out 0x20, al
    pop eax

    iret

go: 
    call dequeue
    mov DWORD [running], eax
    mov esp, [eax]
    pop gs
    pop fs
    pop es
    pop ds
    popad
    iret

outportb:
    push ebp
    mov ebp, esp
    push dx
    push ax
    mov dx, [ebp + 8]
    mov al, [ebp + 12]
    out dx, al
    pop ax
    pop dx
    pop ebp
    ret

init_timer_dev:
    ;1) Do the normal preamble for assembly functions (set up ebp and save any registers
    ;that will be used). The first arg is time in ms
    ;2) move the ms argument value into a register (say, edx)
    ;3) Multiply dx (only the bottom 16 bits will be used) by 1193. 
    ;Why? because the timer cycles at 1193180 times a second, which is 1193 times a ms 
    ;note: The results must fit in 16 bits, so ms can't be more than 54.
    ;So, put your code for steps 1 - 3 HERE:
    push ebp
    mov ebp, esp
    pushfd
    pushad
    mov edx, [ebp + 8]
    mov ax, 1193 
    imul dx, ax

    ;The code for steps 4-6 is given to you for free below...
    ;4) Send the command word to the PIT (Programmable Interval Timer) that initializes Counter 0
    ;(there are three counters, but you will only use counter 0).
    ;The PIT will be initalized to mode 3, Read or Load LSB first then MSB, and
    ;Channel (counter) 0 with the following bits: 0b00110110 =
    ;Counter 0 |Read then load|Mode 3|Binary. So, the instructions will be:
    mov al, 0b00110110 ;0x43 is the Write control word
    out 0x43, al
    ;5) Load the LSB first then the MSB.
    ;0x40 = Load counter 0 with the following code: 
    mov ax, dx
    out 0x40, al ;LSB
    xchg ah, al
    out 0x40, al ;MSB
    ;6) clean up (pop ebp and other regs used) and return
    popad
    popfd
    pop ebp
    ret
    