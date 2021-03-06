// Saved registers for kernel context switches.
struct context {
  uint64 ra;
  uint64 sp;
  // callee-saved
  uint64 s0;
  uint64 s1;
  uint64 s2;
  uint64 s3;
  uint64 s4;
  uint64 s5;
  uint64 s6;
  uint64 s7;
  uint64 s8;
  uint64 s9;
  uint64 s10;
  uint64 s11;
};

#define NTHREAD 8

// Per-CPU state.
struct cpu {
  struct proc *proc;          // The process running on this cpu, or null.
  struct context context;     // swtch() here to enter scheduler().
  int noff;                   // Depth of push_off() nesting.
  int intena;                 // Were interrupts enabled before push_off()?

  /// task 3 - threads
  struct thread *thread;    // The thread running on this cpu, or null.
};

extern struct cpu cpus[NCPU];

// per-process data for the trap handling code in trampoline.S.
// sits in a page by itself just under the trampoline page in the
// user page table. not specially mapped in the kernel page table.
// the sscratch register points here.
// uservec in trampoline.S saves user registers in the trapframe,
// then initializes registers from the trapframe's
// kernel_sp, kernel_hartid, kernel_satp, and jumps to kernel_trap.
// usertrapret() and userret in trampoline.S set up
// the trapframe's kernel_*, restore user registers from the
// trapframe, switch to the user page table, and enter user space.
// the trapframe includes callee-saved user registers like s0-s11 because the
// return-to-user path via usertrapret() doesn't return through
// the entire kernel call stack.
struct trapframe {
  /*   0 */ uint64 kernel_satp;   // kernel page table
  /*   8 */ uint64 kernel_sp;     // top of process's kernel stack
  /*  16 */ uint64 kernel_trap;   // usertrap()
  /*  24 */ uint64 epc;           // saved user program counter
  /*  32 */ uint64 kernel_hartid; // saved kernel tp
  /*  40 */ uint64 ra;
  /*  48 */ uint64 sp;
  /*  56 */ uint64 gp;
  /*  64 */ uint64 tp;
  /*  72 */ uint64 t0;
  /*  80 */ uint64 t1;
  /*  88 */ uint64 t2;
  /*  96 */ uint64 s0;
  /* 104 */ uint64 s1;
  /* 112 */ uint64 a0;
  /* 120 */ uint64 a1;
  /* 128 */ uint64 a2;
  /* 136 */ uint64 a3;
  /* 144 */ uint64 a4;
  /* 152 */ uint64 a5;
  /* 160 */ uint64 a6;
  /* 168 */ uint64 a7;
  /* 176 */ uint64 s2;
  /* 184 */ uint64 s3;
  /* 192 */ uint64 s4;
  /* 200 */ uint64 s5;
  /* 208 */ uint64 s6;
  /* 216 */ uint64 s7;
  /* 224 */ uint64 s8;
  /* 232 */ uint64 s9;
  /* 240 */ uint64 s10;
  /* 248 */ uint64 s11;
  /* 256 */ uint64 t3;
  /* 264 */ uint64 t4;
  /* 272 */ uint64 t5;
  /* 280 */ uint64 t6;
};

// original
//enum procstate { UNUSED, USED, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

enum procstate { UNUSED, USED, ZOMBIE };

enum threadstate {
    UNUSED_T, USED_T, SLEEPING, RUNNABLE, RUNNING, ZOMBIE_T,STOPPED, INVALID
};


struct thread {
//    struct spinlock lock;
    // t->lock must be held when using these:
    enum threadstate state;       // Thread state
    void *chan;                  // If non-zero, sleeping on chan
    int killed;                  // If non-zero, have been killed
    int xstate;                  // Exit status to be returned to parent's wait
    int tid;                    // thread ID
    struct proc *parent;        // ptr to the process that holds this thread
    int should_exit;             // If non-zero, thread should preform exit
    int blocked_on_semaphore;


    //  these are private to the threads, so t->lock need not be held.
    uint64 kstack;               // Virtual address of kernel stack
    struct trapframe *trapframe;        // Trap frame for current syscall
    struct context context;     // swtch() here to run thread
    struct trapframe* usertrap_backup;
    int t_index;
};


// Per-process state
struct proc {
  struct spinlock lock;

  // p->lock must be held when using these:
  enum procstate state;        // Process state
//  void *chan;                  // If non-zero, sleeping on chan
  int killed;                  // If non-zero, have been killed
  int xstate;                  // Exit status to be returned to parent's wait
  int pid;                     // Process ID
  // proc_tree_lock must be held when using this:
  struct proc *parent;         // Parent process
  int in_usr_sig_handler;      // If non-zero, process is currently handling signals
  int handaling_exit;         // If non-zero, process is currently handling signals

  // these are private to the process, so p->lock need not be held.
//  uint64 kstack;               // Virtual address of kernel stack
  uint64 sz;                   // Size of process memory (bytes)
  pagetable_t pagetable;       // User page table
//  struct trapframe *trapframe; // data page for trampoline.S
//  struct context context;      // swtch() here to run process
  struct file *ofile[NOFILE];  // Open files
  struct inode *cwd;           // Current directory
  char name[16];               // Process name (debugging)


  // signal data structure
  uint pending_signals; // Pending signals array
  uint signal_mask;  // Signal masks
  uint signal_mask_backup;
  uint signal_mask_arr[32]; // Signal masks array for each handler
  void* signal_handlers[32]; // Signal handlers
//  struct trapframe* usertrap_backup;
  uint frozen; // 0 not frozen, 1 frozen

  struct thread threads[NTHREAD];   // thread array
  int threads_num;               // number of current threads in array
};




