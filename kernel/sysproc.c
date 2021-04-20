#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void) {
    int n;
    if (argint(0, &n) < 0)
        return -1;
    exit(n);
    return 0;  // not reached
}

uint64
sys_getpid(void) {
    return myproc()->pid;
}

uint64
sys_fork(void) {
    return fork();
}

uint64
sys_wait(void) {
    uint64 p;
    if (argaddr(0, &p) < 0)
        return -1;
    return wait(p);
}

uint64
sys_sbrk(void) {
    int addr;
    int n;

    if (argint(0, &n) < 0)
        return -1;
    addr = myproc()->sz;
    if (growproc(n) < 0)
        return -1;
    return addr;
}

uint64
sys_sleep(void) {
    int n;
    uint ticks0;

    if (argint(0, &n) < 0)
        return -1;
    acquire(&tickslock);
    ticks0 = ticks;
    while (ticks - ticks0 < n) {
        if (myproc()->killed) {
            release(&tickslock);
            return -1;
        }
        sleep(&ticks, &tickslock);
    }
    release(&tickslock);
    return 0;
}

uint64
sys_kill(void) {
    int pid;

    if (argint(0, &pid) < 0)
        return -1;
    return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void) {
    uint xticks;

    acquire(&tickslock);
    xticks = ticks;
    release(&tickslock);
    return xticks;
}

/// Task 2.1.3
uint64 sys_sigprocmask(void) {
    uint new_mask;
    if (argint(0, &new_mask) < 0)
        return -1;
    old_mask = myproc()->signal_mask;
    myproc()->signal_mask = new_mask;
    return old_mask;
}

/// Task 2.1.4
uint64 sys_sigaction(void) {
    int signum;
    if (argint(0, &signum) < 0 || signum < 0 || signum > 31)
        return -1;
    if(signum == SIGKILL || signum == SIGSTOP)
        return -1;
    uint64 act_ptr;
    if (argaddr(0, &act_ptr) < 0 || act_ptr == 0)
        return -1;
    uint64 oldact_ptr;
    if (argaddr(0, &oldact_ptr) < 0 || oldact_ptr == 0)
        return -1;

    return sigaction(signum, (const struct sigaction*) act_ptr, (struct sigaction*) oldact_ptr);
}

