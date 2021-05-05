#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

struct cpu cpus[NCPU];

struct proc proc[NPROC];

struct proc *initproc;

int nextpid = 1;
int nexttid = 1;
struct spinlock pid_lock;
struct spinlock tid_lock;


extern void forkret(void);

extern void sigret_start(void);

extern void sigret_end(void);

static void freeproc(struct proc *p);

static void freethread(struct thread *t);

extern char trampoline[]; // trampoline.S

// helps ensure that wakeups of wait()ing
// parents are not lost. helps obey the
// memory model when using p->parent.
// must be acquired before any p->lock.
struct spinlock wait_lock;

//void
//proc_mapstacks(pagetable_t kpgtbl) {
//    struct proc *p;
//    struct thread *t;
//    for (p = proc; p < &proc[NPROC]; p++) {
//        for (t = p->threads; t < &p->threads[NTHREAD]; t++){
//            char *pa = kalloc();
//            if (pa == 0)
//                panic("kalloc");
////        uint64 va = KSTACK((int) (p - proc));
//            uint64 va = KSTACK(NTHREAD * (p - proc) + (t - p->threads));
//            kvmmap(kpgtbl, va, (uint64) pa, PGSIZE, PTE_R | PTE_W);
//        }
//    }
//}

// Allocate a page for each process's kernel stack.
// Map it high in memory, followed by an invalid
// guard page.
void
proc_mapstacks(pagetable_t kpgtbl) {
    struct proc *p;
    for (p = proc; p < &proc[NPROC]; p++) {
        char *pa = kalloc();
        if (pa == 0)
            panic("kalloc");
        uint64 va = KSTACK((int) (p - proc));
        kvmmap(kpgtbl, va, (uint64) pa, PGSIZE, PTE_R | PTE_W);
    }
}

// initialize the proc table at boot time.
void
procinit(void) {
    struct proc *p;
//    struct thread *t;
    initlock(&pid_lock, "nextpid");
    initlock(&pid_lock, "nexttid");
    initlock(&wait_lock, "wait_lock");
    for (p = proc; p < &proc[NPROC]; p++) {
        initlock(&p->lock, "proc");
        p->threads[0].kstack = KSTACK((int) (p - proc));
//        for (t = p->threads; t < &p->threads[NTHREAD]; t++) {
//            t->kstack = KSTACK(NTHREAD * (p - proc) + (t - p->threads));
//        }
//        p->kstack = KSTACK((int) (p - proc));

    }
}

//// initialize the thread table at boot time.
//void
//threadinit(proc p) {
//    struct thread *t;
//    for (t = p.threads; t < &p.threads[NTHREADS]; t++) {
//        initlock(&t->lock, "thread");
//        t->kstack = KSTACK((int) (t - thread));
//    }
//}

// Must be called with interrupts disabled,
// to prevent race with process being moved
// to a different CPU.
int
cpuid() {
    int id = r_tp();
    return id;
}

// Return this CPU's cpu struct.
// Interrupts must be disabled.
struct cpu *
mycpu(void) {
    int id = cpuid();
    struct cpu *c = &cpus[id];
    return c;
}

// Return the current struct proc *, or zero if none.
struct proc *
myproc(void) {
    push_off();
    struct cpu *c = mycpu();
    struct proc *p = c->proc;
    pop_off();
    return p;
}

// Return the current struct thread *, or zero if none.
struct thread *mythread(void) {
    push_off();
    struct cpu *c = mycpu();
    struct thread *t = c->thread;
    pop_off();
    return t;
}


int
allocpid() {
    int pid;
    acquire(&pid_lock);
    pid = nextpid;
    nextpid = nextpid + 1;
    release(&pid_lock);
    return pid;
}

int
alloctid() {
    int tid;
    acquire(&tid_lock);
    tid = nexttid;
    nexttid = nexttid + 1;
    release(&tid_lock);
    return tid;
}



int
kthread_id() {
    struct proc *p = myproc();
    struct thread *t = mythread();
    if (p != 0 && t != 0) {
        return t->tid;
    }
    return -1;
}

void
kthread_exit(int status) {
    exit_thread(status);
    printf("Thread TID: %d exited successfully\n", mythread()->tid);
}

int
kthread_join(int thread_id, uint64 status) {
    struct proc *p = myproc();
    struct thread *t = mythread();
//    printf("entered kthread_join() -> TID: %d\n",t->tid);
    // thread cannot wait for himself
    if (t->tid == thread_id)
        return -1;
    struct thread *curr_t;
    for (curr_t = p->threads; curr_t < &p->threads[NTHREAD]; curr_t++) {
//        acquire(&p->lock);
        acquire(&wait_lock);
        // found
        if (curr_t->tid == thread_id) {
            while (curr_t->state != ZOMBIE_T) {
                // sleep until curr_t state is changed + release p.lock
//                printf("in kthread_join TID: %d is going to sleep\n",t->tid);
                sleep(curr_t, &wait_lock);
//                printf("in kthread_join TID: %d is EXIT form to sleep\n",t->tid);
            }
            if (curr_t->state == ZOMBIE_T) {
                printf("inside kthread_join TID: %d is ZOMBIE with xstate: %d\n", curr_t->tid,curr_t->xstate);
                acquire(&p->lock);
                if(status != 0 && copyout(p->pagetable, status, (char *)&curr_t->xstate,
                                        sizeof(curr_t->xstate)) < 0) {
                    release(&p->lock);
                    release(&wait_lock);
                    return -1;
                }
                freethread(curr_t);
                release(&p->lock);
                release(&wait_lock);
                return 0;
            }
        }
        release(&wait_lock);
    }
    // not found: not existed or already exited
    return -1;
}
int
kthread_create(uint64 start_func, uint64 stack) {
//    printf("inside kthread_create \n");
    struct proc *p = myproc();
    acquire(&p->lock);
    struct thread *t = allocthread(p);
    // if allocthread failed
    if (t == 0) {
        release(&p->lock);
        return -1;
    }
    if ((t->kstack = (uint64) kalloc()) == 0) {
        freethread(t);
        release(&p->lock);
        return -1;
    }
    // Set up new context to start executing at forkret,
    // which returns to user space.
    memset(&t->context, 0, sizeof(struct context));
    t->context.ra = (uint64) forkret;
    t->context.sp = t->kstack + PGSIZE;

//    memmove(t->trapframe,mythread()->trapframe,sizeof (struct trapframe));

    t->trapframe->sp = (stack + MAX_STACK_SIZE - 16); // user stack pointer
    t->trapframe->epc = start_func;  // user program counter
    t->state = RUNNABLE;

    release(&p->lock);
//    printf("about to exit kthread_create \n");
    return t->tid;
}

/// should lock p->locked before calling this function!!
struct thread *allocthread(struct proc *p) {
    struct thread *t;
    for (t = p->threads; t < &p->threads[NTHREAD]; t++) {
        if (t->state == UNUSED_T) {
            goto found;
        } else if (t->state == ZOMBIE_T) {
            freethread(t);
            goto found;
        }
    }
    // there is no unused thread in p
    release(&p->lock);
    return 0;

    found:
    // else found unused thread
    p->threads_num++;
    t->parent = p;
    t->tid = alloctid(); // thread id is it's number in the array
    t->state = USED_T;
    return t;
}

// Look in the process table for an UNUSED proc.
// If found, initialize state required to run in the kernel,
// and return with p->lock held.
// If there are no free procs, or a memory allocation fails, return 0.
static struct proc *
allocproc(void) {
    struct proc *p;
    for (p = proc; p < &proc[NPROC]; p++) {
        acquire(&p->lock);
        if (p->state == UNUSED) {
            goto found;
        } else {
            release(&p->lock);
        }
    }
    return 0;
    found:
    p->pid = allocpid();
    p->state = USED;
    p->signal_handlers[1] = (void *) SIG_IGN;
    p->signal_handlers[9] = (void *) SIGKILL;
    p->signal_handlers[17] = (void *) SIGSTOP;
    p->signal_handlers[19] = (void *) SIGCONT;
    struct thread *t = allocthread(p);
    if (t == 0) {
        freeproc(p);
        release(&p->lock);
        return 0;
    }
    // Allocate a trapframe page.
    void* trapframe_start;
    void* trapframe_backup_start;
    if ((trapframe_start = (struct trapframe *) kalloc()) == 0) {
        freethread(t);
        release(&p->lock);
        return 0;
    }
    // Allocate a usertrap_backup page.
    if ((trapframe_backup_start = (struct trapframe *) kalloc()) == 0) {
        freethread(t);
        release(&p->lock);
        return 0;
    }
//    printf("trapframe_start: %p\n",trapframe_start);
    // set the right index of trapframe for all threads of this proc.
    int t_index = 0;
    struct thread *curr_t;
    for(curr_t = p->threads; curr_t < &p->threads[NTHREAD]; curr_t++){
        curr_t->t_index = t_index;
        curr_t->trapframe = trapframe_start + sizeof(struct trapframe)*t_index;
        curr_t->usertrap_backup = trapframe_backup_start + sizeof(struct trapframe)*t_index;
        t_index++;
    }
    // An empty user page table.
    p->pagetable = proc_pagetable(p);
    if (p->pagetable == 0) {
        freeproc(p);
        release(&p->lock);
        return 0;
    }
    memset(&t->context, 0, sizeof(struct context));
    t->context.ra = (uint64) forkret;
    t->context.sp = t->kstack + PGSIZE;
    return p;
}

// P->lock must be held when calling freethread.
static void
freethread(struct thread *t) {
//    // TODO: maybe you should not free the kstack of thread 0 in each proc
//    // check if it is the first thread of the proc then do not free his stack
    if (t->parent->threads[0].tid != t->tid && t->kstack)
        kfree((void *) t->kstack);
    // free trapframe & usertrap_backup only if it is the first thread of the proc
    if (t->parent->threads[0].tid == t->tid) {
        if (t->trapframe)
            kfree((void *) t->trapframe);
        t->trapframe = 0;
        if (t->usertrap_backup)
            kfree((void *) t->usertrap_backup);
        t->usertrap_backup = 0;
    }
    t->tid = 0;
    t->parent->threads_num--;
    t->parent = 0;
    t->chan = 0;
    t->killed = 0;
    t->xstate = 0;
    t->state = UNUSED_T;
}

// free a proc structure and the data hanging from it,
// including user pages.
// p->lock must be held.
static void
freeproc(struct proc *p) {
    if (p->pagetable)
        proc_freepagetable(p->pagetable, p->sz);
    p->pagetable = 0;
    p->sz = 0;
    p->pid = 0;
    p->parent = 0;
    p->name[0] = 0;
    p->killed = 0;
    p->xstate = 0;
    p->threads_num = 0;
    /// Task 2.1.2
    p->pending_signals = 0;
    p->signal_mask = 0;
    for (int i = 0; i < 32; i++) {
        p->signal_handlers[i] = (void *) SIG_DFL;
        p->signal_mask_arr[i] = 0;
    }
    p->state = UNUSED;
}

// Create a user page table for a given process,
// with no user memory, but with trampoline pages.
pagetable_t
proc_pagetable(struct proc *p) {
    pagetable_t pagetable;
    // An empty page table.
    pagetable = uvmcreate();
    if (pagetable == 0)
        return 0;

    // map the trampoline code (for system call return)
    // at the highest user virtual address.
    // only the supervisor uses it, on the way
    // to/from user space, so not PTE_U.
    if (mappages(pagetable, TRAMPOLINE, PGSIZE,
                 (uint64) trampoline, PTE_R | PTE_X) < 0) {
        uvmfree(pagetable, 0);
        return 0;
    }

    // map the trapframe just below TRAMPOLINE, for trampoline.S.
    if (mappages(pagetable, TRAPFRAME, PGSIZE,
                 (uint64) (p->threads->trapframe), PTE_R | PTE_W) < 0) {
        uvmunmap(pagetable, TRAMPOLINE, 1, 0);
        uvmfree(pagetable, 0);
        return 0;
    }

    return pagetable;
}

// Free a process's page table, and free the
// physical memory it refers to.
void
proc_freepagetable(pagetable_t pagetable, uint64 sz) {
    uvmunmap(pagetable, TRAMPOLINE, 1, 0);
    uvmunmap(pagetable, TRAPFRAME, 1, 0);
    uvmfree(pagetable, sz);
}

// a user program that calls exec("/init")
// od -t xC initcode
uchar initcode[] = {
        0x17, 0x05, 0x00, 0x00, 0x13, 0x05, 0x45, 0x02,
        0x97, 0x05, 0x00, 0x00, 0x93, 0x85, 0x35, 0x02,
        0x93, 0x08, 0x70, 0x00, 0x73, 0x00, 0x00, 0x00,
        0x93, 0x08, 0x20, 0x00, 0x73, 0x00, 0x00, 0x00,
        0xef, 0xf0, 0x9f, 0xff, 0x2f, 0x69, 0x6e, 0x69,
        0x74, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
};

// Set up first user process.
void
userinit(void) {
    struct proc *p;
    p = allocproc();
    if (p == 0) {
        panic("in userinit() allocproc() failed");
    }
    struct thread *t = p->threads;
    initproc = p;
    // allocate one user page and copy init's instructions
    // and data into it.
    uvminit(p->pagetable, initcode, sizeof(initcode));
    p->sz = PGSIZE;

    // prepare for the very first "return" from kernel to user.
    t->trapframe->epc = 0;      // user program counter
    t->trapframe->sp = PGSIZE;  // user stack pointer

    safestrcpy(p->name, "initcode", sizeof(p->name));
    p->cwd = namei("/");
    t->state = RUNNABLE;
//    printf("in userinit() start release to PID: %d \n", p->pid);
    release(&p->lock);
}

// Grow or shrink user memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n) {
    uint sz;
    struct proc *p = myproc();
//    acquire(&p->lock);
    sz = p->sz;
    if (n > 0) {
        if ((sz = uvmalloc(p->pagetable, sz, sz + n)) == 0) {
            return -1;
        }
    } else if (n < 0) {
        sz = uvmdealloc(p->pagetable, sz, sz + n);
    }
    p->sz = sz;
//    release(&p->lock);
    return 0;
}

// Create a new process, copying the parent.
// Sets up child kernel stack to return as if from fork() system call.
int
fork(void) {
    int i, pid;
    struct proc *np;
    struct proc *p = myproc();
    struct thread *currthread = mythread();
    // Allocate process.
    if ((np = allocproc()) == 0) {
        return -1;
    }
    struct thread *nt = &np->threads[0];

    // Copy user memory from parent to child.
    if (uvmcopy(p->pagetable, np->pagetable, p->sz) < 0) {
        freeproc(np);
        release(&np->lock);
        return -1;
    }
    np->sz = p->sz;
    /// Task 2.1.2
    // Copy parent signals to child
    np->signal_mask = p->signal_mask;
    for (int i = 0; i < 32; i++) {
        np->signal_handlers[i] = p->signal_handlers[i];
        np->signal_mask_arr[i] = p->signal_mask_arr[i];
    }

    // copy saved user registers.
    *(nt->trapframe) = *(currthread->trapframe);

    // Cause fork to return 0 in the child.
    nt->trapframe->a0 = 0;

    // increment reference counts on open file descriptors.
    for (i = 0; i < NOFILE; i++)
        if (p->ofile[i])
            np->ofile[i] = filedup(p->ofile[i]);
    np->cwd = idup(p->cwd);

    safestrcpy(np->name, p->name, sizeof(p->name));

    pid = np->pid;
    release(&np->lock);

    acquire(&wait_lock);

    acquire(&np->lock);
    np->parent = p;
    nt->parent = np;

    release(&wait_lock);

    nt->state = RUNNABLE;

    release(&np->lock);
    return pid;
}

// Pass p's abandoned children to init.
// Caller must hold wait_lock.
void
reparent(struct proc *p) {
    struct proc *pp;
    for (pp = proc; pp < &proc[NPROC]; pp++) {
        if (pp->parent == p) {
            pp->parent = initproc;
            wakeup(initproc);
        }
    }
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait().
void
exit(int status) {
    struct proc *p = myproc();
    struct thread *t = mythread();
    struct thread *curr_t;
    if (t->should_exit) {
        exit_thread(status);
    } else {
        acquire(&p->lock);
        for (curr_t = p->threads; curr_t < &p->threads[NTHREAD]; curr_t++) {
            if (curr_t != 0 && curr_t->tid != t->tid) {
                curr_t->should_exit = 1;
                if (curr_t->state == SLEEPING) {
                    curr_t->state = RUNNABLE;
                }
            }
        }
        release(&p->lock);
    }
    // waiting for all other thread of this proc to become ZOMBIE gracefully
    for (;;) {
        int active_thread = 0;
        acquire(&p->lock);
        for (curr_t = p->threads; curr_t < &p->threads[NTHREAD]; curr_t++) {
            if (curr_t->tid != t->tid && curr_t->state != ZOMBIE_T && curr_t->state != UNUSED_T) {
                active_thread = 1;
            }
        }
        if (active_thread) {
            release(&p->lock);
            yield();
        } else {
            // mythread() is the last active in this proc
            release(&p->lock);
            exit_process(status);
        }
    }
}


void
exit_thread(int status) {
    struct thread *t = mythread();
    struct proc *p = myproc();
    struct thread *curr_t;
    acquire(&wait_lock);
    wakeup(t);
    release(&wait_lock);
    acquire(&p->lock);
    int active_thread = 0;
    for (curr_t = p->threads; curr_t < &p->threads[NTHREAD]; curr_t++) {
        if (curr_t->tid != t->tid && curr_t->state != ZOMBIE_T && curr_t->state != UNUSED_T) {
            active_thread = 1;
        }
    }
    t->xstate = status;
    t->state = ZOMBIE_T;
    if (!active_thread) {
        // mythread() is the last active in this proc
        release(&p->lock);
        exit_process(status);
    }
//    printf("inside exit_thread TID: %d turn into ZOMBIE and going to sched\n",t->tid);
    sched();
    panic("zombie exit");
}

void
exit_process(int status) {
    struct proc *p = myproc();
    struct thread *t = mythread();
    if (p == initproc)
        panic("init exiting");
    // Close all open files.
    // TODO: make sure we close all active file only after all thread of this process exit
    for (int fd = 0; fd < NOFILE; fd++) {
        if (p->ofile[fd]) {
            struct file *f = p->ofile[fd];
            fileclose(f);
            p->ofile[fd] = 0;
        }
    }
    begin_op();
    iput(p->cwd);
    end_op();
    p->cwd = 0;
    acquire(&wait_lock);
    // Give any children to init.
    reparent(p);
    // Parent might be sleeping in wait().
    wakeup(p->parent);
    acquire(&p->lock);
    p->xstate = status;
//    p->killed = 1;
    p->state = ZOMBIE;
    t->xstate = status;
    t->state = ZOMBIE_T;
    release(&wait_lock);
    // Jump into the scheduler, never to return.
    sched();
    panic("zombie exit");

}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(uint64 addr) {
    struct proc *np;
    int havekids, pid;
    struct proc *p = myproc();
    acquire(&wait_lock);
    for (;;) {
        // Scan through table looking for exited children.
        havekids = 0;
        for (np = proc; np < &proc[NPROC]; np++) {
            if (np->parent == p) {
                // make sure the child isn't still in exit() or swtch().
                acquire(&np->lock);
                havekids = 1;
                if (np->state == ZOMBIE) {
                    // Found one.
                    pid = np->pid;
                    if (addr != 0 && copyout(p->pagetable, addr, (char *) &np->xstate,
                                             sizeof(np->xstate)) < 0) {
                        release(&np->lock);
                        release(&wait_lock);
                        return -1;
                    }
                    struct thread *curr_t;
                    for (curr_t = np->threads; curr_t < &np->threads[NTHREAD]; curr_t++) {
                        if (curr_t->state == ZOMBIE_T) {
                            // t -> lock mush be locked when calling freethread
                            freethread(curr_t);
                        }
                        if (curr_t->state != ZOMBIE_T && curr_t->state != UNUSED_T) {
                            panic("in Wait() Process zombie have thread that is not ZOMBIE_T or UNUSED_T");
                        }
                    }
                    freeproc(np);
                    release(&np->lock);
                    release(&wait_lock);
                    return pid;
                }
                release(&np->lock);
            }
        }

        // No point waiting if we don't have any children.
        if (!havekids || p->killed) {
            release(&wait_lock);
            return -1;
        }
        // Wait for a child to exit.
        sleep(p, &wait_lock);  //DOC: wait-sleep
    }
}

// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run.
//  - swtch to start running that process.
//  - eventually that process transfers control
//    via swtch back to the scheduler.
void
scheduler(void) {
    struct proc *p;
    struct cpu *c = mycpu();
    struct thread *t;
    c->proc = 0;
    c->thread = 0;
    for (;;) {
        // Avoid deadlock by ensuring that devices can interrupt.
        intr_on();
        int found = 0;
        for (p = proc; p < &proc[NPROC]; p++) {
            acquire(&p->lock);
            if (p->state == USED) {
                for (t = p->threads; t < &p->threads[NTHREAD]; t++) {
                    if (t->state == RUNNABLE) {
//                        if (t->tid == 4) {
//                            // TODO: why we goes here it should_exit = 1 ?
//                            printf("tid 4 scehdulad to run\n");
//                        }
                        // Switch to chosen process.  It is the process's job
                        // to release its lock and then reacquire it
                        // before jumping back to us.
                        t->state = RUNNING;
                        c->proc = p;
                        c->thread = t;
                        swtch(&c->context, &t->context);
                        // Process is done running for now.
                        // It should have changed its p->state before coming back.
                        c->proc = 0;
                        c->thread = 0;
                        found = 1;
                    }
                }
            }
            release(&p->lock);
        }
        if (found == 0) {
            intr_on();
            asm volatile("wfi");
        }
    }
}

// Switch to scheduler.  Must hold only p->lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->noff, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void) {
    int intena;
    struct proc *p = myproc();
    struct thread *t = mythread();
    if (!holding(&p->lock)) {
        panic("sched p->lock");
    }
    // checl if the current CPU holds only 1 lock
    if (mycpu()->noff != 1)
        panic("sched locks");
    if (t->state == RUNNING)
        panic("sched running");
    if (intr_get())
        panic("sched interruptible");
    intena = mycpu()->intena;
    swtch(&t->context, &mycpu()->context);
    mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void) {
    struct proc *p = myproc();
    struct thread *t = mythread();
    acquire(&p->lock);
    t->state = RUNNABLE;
    sched();
    release(&p->lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch to forkret.
void
forkret(void) {
    static int first = 1;
    // Still holding p->lock from scheduler.
    release(&myproc()->lock);
    if (first) {
        // File system initialization must be run in the context of a
        // regular process (e.g., because it calls sleep), and thus cannot
        // be run from main().
        first = 0;
        fsinit(ROOTDEV);
    }
    usertrapret();
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk) {
    struct proc *p = myproc();
    struct thread *t = mythread();
    // Must acquire p->lock in order to
    // change p->state and then call sched.
    // Once we hold p->lock, we can be
    // guaranteed that we won't miss any wakeup
    // (wakeup locks p->lock),
    // so it's okay to release lk.

    acquire(&p->lock);  //DOC: sleeplock1
    release(lk);

    // Go to sleep.
    t->chan = chan;
    t->state = SLEEPING;

    sched();

    // Tidy up.
    t->chan = 0;
    // Reacquire original lock.
    release(&p->lock);
    acquire(lk);
}

// Wake up all processes sleeping on chan.
// Must be called without any p->lock.
void
wakeup(void *chan) {
    struct proc *p;
    struct thread *t;
    for (p = proc; p < &proc[NPROC]; p++) {
        if (p->state == USED) {
            for (t = p->threads; t < &p->threads[NTHREAD]; t++) {
                if (t != mythread()) {
                    acquire(&p->lock);
                    // if t is wakeup when is proc parent is frozen then it should stop
                    if (t->state == SLEEPING && t->parent->frozen && t->chan == chan) {
                        t->state = STOPPED;
                    } else if (t->state == SLEEPING && t->chan == chan) {
                        t->state = RUNNABLE;
                    }
                    release(&p->lock);
                }
            }
        }
    }
}

int
kill(int pid, int signum) {
    struct proc *p;
    for (p = proc; p < &proc[NPROC]; p++) {
        acquire(&p->lock);
        if (p->pid == pid) {
            if (p->state != ZOMBIE && p->state != UNUSED && p->killed != 1) {
                p->pending_signals = p->pending_signals | (1 << signum);
                // if the handler of this signum is SIGCONT turn on the sigcont flag
                if (p->signal_handlers[signum] == (void *) SIGCONT)
                    p->pending_signals = p->pending_signals | (1 << SIGCONT);
                release(&p->lock);
                return 0;
            }
        }
        release(&p->lock);
    }
    return -1;
}

// Kill the process with the given pid.
// The victim won't exit until it tries to return
// to user space (see usertrap() in trap.c).
//int
//kill(int pid) {
//    struct proc *p;
//
//    for (p = proc; p < &proc[NPROC]; p++) {
//        acquire(&p->lock);
//        if (p->pid == pid) {
//            p->killed = 1;
//            if (p->state == SLEEPING) {
//                // Wake process from sleep().
//                p->state = RUNNABLE;
//            }
//            release(&p->lock);
//            return 0;
//        }
//        release(&p->lock);
//    }
//    return -1;
//}

// Copy to either a user address, or kernel address,
// depending on usr_dst.
// Returns 0 on success, -1 on error.
int
either_copyout(int user_dst, uint64 dst, void *src, uint64 len) {
    struct proc *p = myproc();
    if (user_dst) {
        return copyout(p->pagetable, dst, src, len);
    } else {
        memmove((char *) dst, src, len);
        return 0;
    }
}

// Copy from either a user address, or kernel address,
// depending on usr_src.
// Returns 0 on success, -1 on error.
int
either_copyin(void *dst, int user_src, uint64 src, uint64 len) {
    struct proc *p = myproc();
    if (user_src) {
        return copyin(p->pagetable, dst, src, len);
    } else {
        memmove(dst, (char *) src, len);
        return 0;
    }
}

// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
// TODO: add threas support to this one
void
procdump(void) {
    static char *states[] = {
            [UNUSED]    "unused",
            [SLEEPING]  "sleep ",
            [RUNNABLE]  "runble",
            [RUNNING]   "run   ",
            [ZOMBIE]    "zombie"
    };
    struct proc *p;
    char *state;

    printf("\n");
    for (p = proc; p < &proc[NPROC]; p++) {
        if (p->state == UNUSED)
            continue;
        if (p->state >= 0 && p->state < NELEM(states) && states[p->state])
            state = states[p->state];
        else
            state = "???";
        printf("%d %s %s", p->pid, state, p->name);
        printf("\n");
    }
}


/// Task 2.1.4
int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact) {
    struct proc *p = myproc();
    acquire(&p->lock);
    // Save old signal handler and copy it to user-space
    struct sigaction tmp_oldact;
    tmp_oldact.sa_handler = p->signal_handlers[signum];
    tmp_oldact.sigmask = p->signal_mask_arr[signum];
    copyout(p->pagetable, (uint64) oldact, (char *) &tmp_oldact, sizeof(tmp_oldact));
    // Register the new signal handler for the given signal number in the proc
    // copy new act form user-space to kernel-space
    struct sigaction newact;
    either_copyin(&newact, 1, (uint64) act, sizeof(struct sigaction));
    p->signal_handlers[signum] = newact.sa_handler;
    p->signal_mask_arr[signum] = newact.sigmask;
//    printf("in proc.c new act.sa_handler: %d\n", newact.sa_handler);
//    printf("in proc.c new act.sigmask: %d\n", newact.sigmask);
    release(&p->lock);
    return 0;
}


/// Task 2.4
/// Checking for the process's 32 possible pending signals and handling them.
// TODO: shold we lcok the fucnrion so only 1 thread will handle all the each signal each time?
void signal_handler(void) {
    struct proc *p = myproc();
    struct thread *t = mythread();
    int pending_signal_bit;
    int mask_signal_bit;
    if (p == 0 || t == 0) // check if process & thread not NULL
        return;
    for (int i = 0; i < 32; i++) {
        acquire(&p->lock);
        pending_signal_bit = p->pending_signals & (1 << i); // Check if the bit in position i of pend_sig is 1
        mask_signal_bit = p->signal_mask & (1 << i);   // Check if the bit in the i place at the mask is 1
        if (pending_signal_bit && !mask_signal_bit) {  //SIGSTOP and SIGKILL can not be blocked
            if (p->signal_handlers[i] == (void *) SIG_DFL ||
                p->signal_handlers[i] == (void *) SIGKILL) { // SIGKILL Handling
                p->pending_signals ^= (1 << i); // Set the bit of the signal back to zero (bitwise xor)
                release(&p->lock);
                exit(-1);
            } else if (p->signal_handlers[i] == (void *) SIGSTOP) { // SIGKILL Handling
                p->frozen = 1;
                // need to stop all other threads in this proc except mythread() that should wait to be released or killed
                struct thread *curr_t;
                for (curr_t = p->threads; curr_t < &p->threads[NTHREAD]; curr_t++) {
                    if (curr_t->tid != t->tid && (curr_t->state == RUNNING || curr_t->state == RUNNABLE))
                        curr_t->state = STOPPED;
                }
                while ((p->pending_signals & (1 << SIGCONT)) == 0) { // while SIGCONT is not turned on
                    // TODO: should all thread of this proc be frozen or just the current one?
                    release(&p->lock);
                    yield();
                    acquire(&p->lock);
                    // check if SIGKILL is received before SIGCONT
                    if (p->pending_signals & (1 << SIGKILL)) {
                        p->pending_signals ^= (1 << SIGKILL);
                        for (curr_t = p->threads; curr_t < &p->threads[NTHREAD]; curr_t++) {
                            if (curr_t->tid != t->tid && curr_t->state == STOPPED)
                                curr_t->state = RUNNABLE;
                        }
                        release(&p->lock);
                        exit(-1);
                    }
                }
                p->frozen = 0;
                // release all other threads of p form stopped
                for (curr_t = p->threads; curr_t < &p->threads[NTHREAD]; curr_t++) {
                    if (curr_t->tid != t->tid && curr_t->state == STOPPED)
                        curr_t->state = RUNNABLE;
                }
                p->pending_signals ^= (1 << i); // Set the bit of the signal back to zero (bitwise xor)
                p->pending_signals ^= (1 << 19); // Set the bit of the signal back to zero (bitwise xor)
            }
                // just in case SIGCONT received and the process is not on SIGSTOP
            else if (p->signal_handlers[i] == (void *) SIGCONT) { // SIGKILL Handling
                p->pending_signals ^= (1 << i); // Set the bit of the signal back to zero (bitwise xor)
            } else if (p->signal_handlers[i] ==
                       (void *) SIG_IGN) { // The handler of the current signal is IGN, thus we ignore the current signal
                p->pending_signals ^= (1 << i); // Set the bit of the signal back to zero (bitwise xor)
            }
                // Handling user space handler only if the proc is not currently in user-space sig-handler
            else if (!p->in_usr_sig_handler) {
                p->in_usr_sig_handler = 1;
                p->pending_signals ^= (1 << i); // Set the bit of the signal back to zero
                // Backup mask
                p->signal_mask_backup = p->signal_mask;
                p->signal_mask = p->signal_mask_arr[i];
                // Backup trapframe
                memmove(t->usertrap_backup, t->trapframe, sizeof(struct trapframe)); //Copy Backup
                // change return address to sigret function
                t->trapframe->sp -= &sigret_end - &sigret_start; // Save a space at the stack to the sigret function
                copyout(p->pagetable, (uint64) t->trapframe->sp, (char *) sigret_start,
                        &sigret_end - &sigret_start); // memmove function move sigret function to sp register
                t->trapframe->ra = t->trapframe->sp; // Return address is the code on stack
                t->trapframe->a0 = i; // Save signum argument
                t->trapframe->epc = (uint64) p->signal_handlers[i]; // move handler[i] function to epc in order to run the user handler
                release(&p->lock);
                return;
            }
        }
        release(&p->lock);
    }
}



