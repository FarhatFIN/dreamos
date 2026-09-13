; Заглушки CPU-исключений (векторы 0-31) и аппаратных IRQ (32-47).
; Формат кадра на стеке обязан совпадать с struct Registers (src/interrupts.h):
; [ int_no ][ err_code ][ edi..eax ][ ds ][ eip ][ cs ][ eflags ] ...

[bits 32]

extern isr_handler
extern irq_handler

%macro ISR_NOERR 1
global isr%1
isr%1:
    push dword 0                 ; CPU не толкнул код ошибки — добавляем фиктивный,
                                 ; чтобы кадр был одинаковым для всех исключений
    push dword %1
    jmp isr_common
%endmacro

%macro ISR_ERR 1
global isr%1
isr%1:
    push dword %1                ; код ошибки уже на стеке (CPU его положил)
    jmp isr_common
%endmacro

%macro IRQ_STUB 2
global irq%1
irq%1:
    push dword 0
    push dword %2                ; вектор = номер IRQ + 32 (после ремапа PIC)
    jmp irq_common
%endmacro

ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR   8                   ; #DF
ISR_NOERR 9
ISR_ERR   10                  ; #TS
ISR_ERR   11                  ; #NP
ISR_ERR   12                  ; #SS
ISR_ERR   13                  ; #GP
ISR_ERR   14                  ; #PF
ISR_NOERR 15
ISR_NOERR 16
ISR_ERR   17                  ; #AC
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_ERR   21                  ; #CP
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_NOERR 30
ISR_NOERR 31

IRQ_STUB 0,  32
IRQ_STUB 1,  33
IRQ_STUB 2,  34
IRQ_STUB 3,  35
IRQ_STUB 4,  36
IRQ_STUB 5,  37
IRQ_STUB 6,  38
IRQ_STUB 7,  39
IRQ_STUB 8,  40
IRQ_STUB 9,  41
IRQ_STUB 10, 42
IRQ_STUB 11, 43
IRQ_STUB 12, 44
IRQ_STUB 13, 45
IRQ_STUB 14, 46
IRQ_STUB 15, 47

; Общий хвост. Порядок важен: сначала pad из 4 dwords (GCC при tail-call
; оптимизации пишет outgoing-args в [esp+4..] — без pad там оказался бы
; сохранённый ds), и только затем аргумент-указатель на Registers.
isr_common:
    pusha
    push ds
    mov eax, esp                ; eax = адрес слота ds = указатель на Registers
    push dword 0
    push dword 0
    push dword 0
    push dword 0
    push eax
    cld
    call isr_handler
    add esp, 20                 ; снять указатель + pad
    pop ds
    popa
    add esp, 8                  ; снять int_no + err_code
    iretd

irq_common:
    pusha
    push ds
    mov eax, esp
    push dword 0
    push dword 0
    push dword 0
    push dword 0
    push eax
    cld
    call irq_handler
    add esp, 20
    pop ds
    popa
    add esp, 8
    iretd
