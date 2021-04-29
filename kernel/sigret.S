#include "syscall.h"
.globl sigret_start
.globl sigret_end

sigret_start:
	        li a7, SYS_sigret
            ecall
sigret_end: