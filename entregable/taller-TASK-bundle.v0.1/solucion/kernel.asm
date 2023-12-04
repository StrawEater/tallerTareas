; ** por compatibilidad se omiten tildes **
; ==============================================================================
; TALLER System Programming - ORGANIZACION DE COMPUTADOR II - FCEN
; ==============================================================================

%include "print.mac"

global start


; COMPLETAR - Agreguen declaraciones extern según vayan necesitando
extern GDT_DESC
extern screen_draw_layout
extern IDT_DESC
extern idt_init
extern pic_reset
extern pic_enable
extern mmu_init_kernel_dir
extern pruebaCopy
extern mmu_init_task_dir
extern tss_init
extern tasks_screen_draw
extern sched_init
extern tasks_init

; COMPLETAR - Definan correctamente estas constantes cuando las necesiten
%define CS_RING_0_SEL  8
%define DS_RING_0_SEL 24   
%define INIT_TASK_SEL  0xB<<3
%define IDLE_TASK_SEL  0xC<<3


BITS 16
;; Saltear seccion de datos
jmp start

;;
;; Seccion de datos.
;; -------------------------------------------------------------------------- ;;
start_rm_msg db     'Iniciando kernel en Modo Real'
start_rm_len equ    $ - start_rm_msg

start_pm_msg db     'Iniciando kernel en Modo Protegido'
start_pm_len equ    $ - start_pm_msg

;;
;; Seccion de código.
;; -------------------------------------------------------------------------- ;;

;; Punto de entrada del kernel.
BITS 16
start:
    ; COMPLETAR - Deshabilitar interrupciones
    CLI

    ; Cambiar modo de video a 80 X 50
    mov ax, 03h
    int 10h ; set mode 03h
    xor bx, bx
    mov ax, 1112h
    int 10h ; load 8x8 font

    ; COMPLETAR - Imprimir mensaje de bienvenida - MODO REAL
    ; (revisar las funciones definidas en print.mac y los mensajes se encuentran en la
    ; sección de datos)
    print_text_rm start_rm_msg, start_rm_len, 0x15, 10, 10

    ; COMPLETAR - Habilitar A20
    call A20_enable

    ; COMPLETAR - Cargar la GDT
    LGDT [GDT_DESC]
    

    ; COMPLETAR - Setear el bit PE del registro CR0
    mov eax, CR0
    or eax,1
    mov cr0, eax

    ; COMPLETAR - Saltar a modo protegido (far jump)
    ; (recuerden que un far jmp se especifica como jmp CS_selector:address)
    ; Pueden usar la constante CS_RING_0_SEL definida en este archivo
    jmp CS_RING_0_SEL:modo_protegido

BITS 32
modo_protegido:
    ; COMPLETAR - A partir de aca, todo el codigo se va a ejectutar en modo protegido
    ; Establecer selectores de segmentos DS, ES, GS, FS y SS en el segmento de datos de nivel 0
    ; Pueden usar la constante DS_RING_0_SEL definida en este archivo

    mov ax, DS_RING_0_SEL
    mov DS, ax
    mov ES, ax
    mov GS, ax
    mov FS, ax
    mov SS, ax
    
    mov ebp,0x25000
    mov esp,0x25000

    


    print_text_pm start_pm_msg, start_pm_len, 0x15, 10, 10

    ; COMPLETAR - Establecer el tope y la base de la pila

    ; COMPLETAR - Imprimir mensaje de bienvenida - MODO PROTEGIDO

    ; COMPLETAR - Inicializar pantalla
    
    
    call screen_draw_layout

    call idt_init
    LIDT [IDT_DESC]

    call mmu_init_kernel_dir
    mov CR3, eax
    mov eax, CR0
    or eax,0x80000000
    mov cr0, eax  

    call tss_init
    call sched_init

    call tasks_init    

    call tasks_screen_draw

    mov ax, INIT_TASK_SEL
    ltr ax 

    call pic_reset
    call pic_enable    
    
    ; El PIT (Programmable Interrupt Timer) corre a 1193182Hz.
    ; Cada iteracion del clock decrementa un contador interno, cuando éste llega 
    ; a cero se emite la interrupción. El valor inicial es 0x0 que indica 65536, 
    ; es decir 18.206 Hz
    mov ax, 0x18
    out 0x40, al
    rol ax, 8
    out 0x40, al

    jmp IDLE_TASK_SEL:0x000    

    

    ; sub esp, 8
    ; mov eax, CR3
    ; push eax
    ; push 0x18000
    
    ; call mmu_init_task_dir
    ; mov CR3, eax
    
    ; pop eax
    ; pop eax
    ; add esp, 8
    

    ; ;mov DWORD [0x7000000], 0xFAFACACA
    ; ;mov DWORD [0x7000004], 0xBEBED0D0

    ; mov CR3, eax

    ; xor eax, eax
    ; int 88
    ; int 98

    ; Ciclar infinitamente 
    mov eax, 0xFFFF
    mov ebx, 0xFFFF
    mov ecx, 0xFFFF
    mov edx, 0xFFFF
    jmp $

;; -------------------------------------------------------------------------- ;;

%include "a20.asm"
