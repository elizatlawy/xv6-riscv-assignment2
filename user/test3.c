#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/syscall.h"
#include "kernel/param.h"

void
thread_test()
{
    int tid =  kthread_id();
    sleep(100);
    printf("Hello World! tid: %d\n", tid);
    kthread_exit(tid);
}

void
thread_test3()
{
    int tid;
    int ids[8];
    void* stacks[8];
    for(int i = 0; i < 8; i++){
        void* stack = malloc(MAX_STACK_SIZE);
        stacks[i] = stack;
        ids[i] = kthread_create(thread_test, stack);
        sleep(3);
    }
    // kthread_exit(0);  // exit process
    tid = kthread_id();
    printf("running thread is: %d\n", tid);

    for(int i = 0; i<8;i++){
        if(ids[i]!=-1){
            kthread_join(ids[i],0);
            free(stacks[i]);
        }
        else{
            printf("try %d fail\n",i);
        }
    }

    kthread_exit(0); // exit process
    // exit(0);

}