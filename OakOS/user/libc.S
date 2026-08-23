.section .text
.global oak_getpid
oak_getpid:
    mov $2, %eax
    int $0x80
    ret

.global oak_fork
oak_fork:
    mov $7, %eax
    int $0x80
    ret

.global oak_waitpid
oak_waitpid:
    mov $5, %eax
    int $0x80
    ret

.global oak_exit
oak_exit:
    mov $4, %eax
    int $0x80
    ret

.global oak_write
oak_write:
    mov $1, %eax
    int $0x80
    ret

.global oak_mkdir
oak_mkdir:
    mov $8, %eax
    int $0x80
    ret

.global oak_create
oak_create:
    mov $9, %eax
    int $0x80
    ret

.global oak_write_file
oak_write_file:
    mov $10, %eax
    int $0x80
    ret

.global oak_read_file
oak_read_file:
    mov $11, %eax
    int $0x80
    ret

.section .note.GNU-stack,"",@progbits