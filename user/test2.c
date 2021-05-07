#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/syscall.h"
#include "kernel/param.h"


void test_thread1() {
    printf("first Thread is now running\n");
    kthread_exit(1);
}

void test_thread2() {
    printf("second Thread is now running\n");
    kthread_exit(2);
}


void
thread_test1() {
    fprintf(2, "main function\n");
    int tid[2];
    int status1;
    int status2;
    void *stack1 = malloc(MAX_STACK_SIZE);
    void *stack2 = malloc(MAX_STACK_SIZE);
    tid[0] = kthread_create(test_thread1, stack1);
    tid[1] = kthread_create(test_thread2, stack2);
    printf("after create before join, new tid : %d \n", tid[0]);
    printf("after create before join, new tid : %d \n", tid[1]);
    kthread_join(tid[0], &status1);
    kthread_join(tid[1], &status2);
    free(stack1);
    free(stack2);
    printf("Finished testing threads, thread id: %d, status: %d\n", tid[0], status1);
    printf("Finished testing threads, thread id: %d, status: %d\n", tid[1], status2);
    kthread_exit(3);
}


void
thread_test2(void) {
    int tid;
    int status;
    int ids[7];
    void *stacks[7];
    for (int i = 0; i < 7; i++) {
        void *stack = malloc(MAX_STACK_SIZE);
        ids[i] = tid;
        stacks[i] = stack;
        tid = kthread_create(test_thread2, stack);
        kthread_join(tid, &status);
        printf("joined with: %d exit status: %d\n", tid, status);
//        printf("new thread in town: %d\n", tid);
        sleep(3);
    }
    // kthread_exit(0);  // exit process

    for (int i = 0; i < 7; i++) {
        tid = ids[i];
//        kthread_join(tid, &status);
//        printf("joined with: %d exit status: %d\n", tid, status);
        free(stacks[i]);
    }

//    for(int i = 0; i < 7; i++){
//        void* stack = malloc(MAX_STACK_SIZE);
//        tid = kthread_create(test_thread, stack);
//        ids[i] = tid;
//        stacks[i] = stack;
//        printf("new thread in town: %d\n", tid);
//        sleep(3);
//    }
//
//    for(int i = 0; i < 7; i++){
//        tid = ids[i];
//        int join_ret_val = 0;
//        join_ret_val = kthread_join(tid, &status);
//        printf("joined with: %d exit status: %d join ret val is: %d \n", tid, status, join_ret_val);
//        free(stacks[i]);
//    }

    tid = kthread_id();
    printf("running thread is: %d\n", tid);

    kthread_exit(tid); // exit process
    // exit(0);
}


void thread_func(void) {
    int tid = kthread_id();
    printf("Hello World! tid: %d\n", tid);
    sleep(10);
    printf("After sleep TID: %d is about to exit\n", tid);
    kthread_exit(tid);
}

void
thread_test3(void) {
    int tid;
    int status;
    int ids[8];
    void *stacks[8];
    for (int i = 0; i < 7; i++) {
        void *stack = malloc(MAX_STACK_SIZE);
        ids[i] = tid;
        stacks[i] = stack;
        tid = kthread_create(thread_func, stack);
        printf("new thread in town: %d\n", tid);
    }
    sleep(5);
    void *stack = malloc(MAX_STACK_SIZE);
    ids[7] = tid;
    stacks[7] = stack;
    tid = kthread_create(thread_func, stack);
    printf("the 9 thread CREATED: %d\n", tid);

    for (int i = 0; i < 8; i++) {
        tid = ids[i];
        kthread_join(tid, &status);
        printf("joined with: %d exit status: %d\n", tid, status);
        free(stacks[i]);
    }
    for (int i = 0; i < 8; i++) {
        tid = ids[i];
        free(stacks[i]);
    }
    tid = kthread_id();
    printf("running thread is: %d\n", tid);
    kthread_exit(tid); // exit process
    // exit(0);
}

void many_proc_many_thread() {
    int father_pid = getpid();
    int childs_pid[10];
    for (int i = 1; i < 50; i++) {
        childs_pid[i] = fork();
        if (childs_pid[i] < 0) {
            printf("fork failed\n");
            exit(1);
        }
        if (childs_pid[i] == 0) { // child
            thread_test3();
        }
    }
    if (getpid() == father_pid) {
        for (int i = 1; i < 50; i++) {
            int status;
            wait(&status);
            printf("Child PID: %d exit with status: %d\n", childs_pid[i], status);
        }
        printf("FINISHED ALLL\n");
        exit(0);
    }
}

void test_thread3(void) {
    int tid = kthread_id();
    printf("Hello World! tid: %d\n", tid);
    sleep(100);
    printf("After sleep TID: %d is about to exit\n", tid);
    kthread_exit(tid);
}

void kill_main_thread(){
    int tid;
//    int status;
    int ids[7];
    void *stacks[7];
    for (int i = 0; i < 7; i++) {
        void *stack = malloc(MAX_STACK_SIZE);
        ids[i] = tid;
        stacks[i] = stack;
        tid = kthread_create(thread_func, stack);
                printf("new thread in town: %d\n", tid);
        sleep(3);
    }
     kthread_exit(0);  // exit process
    printf("Killed?\n");
    for (int i = 0; i < 7; i++) {
        tid = ids[i];
//        kthread_join(tid, &status);
//        printf("joined with: %d exit status: %d\n", tid, status);
        free(stacks[i]);
    }
}

void test_sigstop(void) {
    int tid = kthread_id();
    printf("Hello World! tid: %d\n", tid);
    if(tid == 9){
        for (int i = 0; i < 100; i++)
            printf("%d\n",i);
        sleep(1);
    }
    sleep(10);
    printf("After sleep TID: %d is about to exit\n", tid);
    kthread_exit(tid);
}


void threads_sigstop(){
    int child_pid = fork();
    if(child_pid < 0){
        printf("frok failed\n");
    }
    else if(child_pid > 0){ // father
        sleep(5);
        int kill_res = 0;
        kill_res = kill(child_pid, 17); // SIGSTOP
        printf(" SIGSTOP sent to child: %d with res: %d\n", child_pid, kill_res);
        sleep(10);
        kill_res = kill(child_pid, 19); // SIGCONT
        printf(" SIGCONT sent to child: %d with res: %d\n", child_pid, kill_res);
        sleep(5);
        kill_res = kill(child_pid, 9); // SIGKILL
        printf(" SIGKILL sent to child: %d with res: %d\n",child_pid,kill_res);
        sleep(50);
        kthread_exit(0); // exit process
    }
    else{ // child proc
        int tid;
        int status;
        int ids[7];
        void *stacks[7];
        for (int i = 0; i < 7; i++) {
            void *stack = malloc(MAX_STACK_SIZE);
            ids[i] = tid;
            stacks[i] = stack;
            tid = kthread_create(test_sigstop, stack);
            printf("new thread in town: %d\n", tid);
            sleep(3);
        }
        for (int i = 0; i < 7; i++) {
            tid = ids[i];
            kthread_join(tid, &status);
            printf("joined with: %d exit status: %d\n", tid, status);
            free(stacks[i]);
        }
        kthread_exit(0); // exit process
    }
}



int
main(int argc, char **argv) {
//    kill_main_thread();
    threads_sigstop();
    printf("got here : bad\n");
    exit(0);
}