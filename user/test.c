#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"
#include "kernel/syscall.h"
#include "kernel/param.h"
#include "Csemaphore.h"



int flag = 0;

void
do_stuff(int a) {
    flag = 1;
}

void
do_stuff2(int a) {
    flag = 1;
}


void
stopcont() {
    struct sigaction *sa = (struct sigaction *) malloc(sizeof(struct sigaction));
    sa->sigmask = 0;
    sa->sa_handler = do_stuff2;

    printf("addr is %d\n", do_stuff);
    printf("addr2 is %d\n", do_stuff2);
    sigaction(10, sa, 0);

    int id_child = fork();
    if (id_child < 0) {
        printf("fork failed\n");
        exit(1);
    }
    if (id_child) { //father
        kill(id_child, 10);
        wait(0);
    } else {
        sleep(20);
//        if (!flag) exit(1);
//        printf("Back\n");
        if (flag)
            printf("Flag is 1\n");
        else
            printf("Flag is 0\n");
    }
    wait(0);

    // if(!flag) exit(1);
    printf("exit, PID: %d \n", id_child);
    exit(0);
}

void sigprocmastk_test() {
    uint newmask = (1 << 9);
    int oldmask = sigprocmask(newmask);
    printf("parent old mask %d\n", oldmask);
    int pid = fork();
    if (pid == 0) {
        uint newmask = (1 << 17);
        uint oldmask = sigprocmask(newmask);
        printf("child old mask %d\n", oldmask);
    }
}



//void sigaction_test() {
//    struct sigaction act;
////    int sig_kill = 9;
//    act.sa_handler = handler2; // put the address of func handler
////    act.sa_handler = (void*)9;
//    act.sigmask = (1 << 9);
//    printf("handler address: %d\n", act.sa_handler);
//    printf("act sigmask: %d\n", act.sigmask);
//    struct sigaction old_act;
//    sigaction(15, &act, &old_act);
//    printf("old act sigmask %d\n", old_act.sigmask);
//    printf("old act sigmask %d\n", old_act.sa_handler);
//    struct sigaction act2;
//    act2.sigmask = (1 << 9);
//    act2.sa_handler = do_stuff2;
//    printf("do_stuff2 address: %d\n", act2.sa_handler);
//    printf("act2 sigmask: %d\n", act2.sigmask);
//    struct sigaction old_act2;
//    sigaction(15, &act, &old_act2);
//    printf("old act2 sigmask %d\n", old_act2.sigmask);
//    printf("old act2 address %d\n", old_act2.sa_handler);
//}

void kill_test() {
    int child_pid = fork();
    if (child_pid < 0) {
        printf("fork failed\n");
        exit(1);
    }
    if (child_pid) { //father
        kill(child_pid, 9);
        int status;
        wait(&status);
        printf("exit wait -> child PID: %d killed with status: %d \n", child_pid, status);
    } else { // child
        sleep(10);
    }
}

void many_kills() {
    for (int i = 0; i < 3; i++) {
        int child_pid = fork();
        if (child_pid == 0)
            sleep(10);
    }
}

void fork_test(){
    int child_pid = fork();
    if (child_pid < 0) {
        printf("fork failed\n");
    }
    else if (child_pid > 0) { // father
//        printf("new child PID is: %d\n", child_pid);
        int status;
        wait(&status);
        printf("Child PID: %d exit with status: %d\n",child_pid, status);
    } else { // child
        printf("new child created\n");
    }
}

void handler2(int x) {
    printf("2\n");
}

void handler3(int x) {
    printf("3\n");
}

void handler4(int x) {
    printf("4\n");
}

void signal_handler_kernel_sig_test(){
    int child_pid = fork();
    if (child_pid < 0) {
        printf("fork failed\n");
    }
    else if (child_pid > 0) { // father
        sleep(20);
        int kill_res = -9;
        sleep(10);
        sleep(20);
        kill_res = kill(child_pid, 17); // SIGSTOP
        printf(" SIGSTOP sent to child: %d with res: %d\n",child_pid,kill_res);
        sleep(10);
        kill_res = kill(child_pid, 19); // SIGCONT
        printf(" SIGCONT sent to child: %d with res: %d\n",child_pid,kill_res);
//        kill_res = kill(child_pid, 9); // SIGKILL
//        printf(" SIGKILL sent to child: %d with res: %d\n",child_pid,kill_res);
        int status;
        wait(&status);
        printf("Child PID: %d exit with status: %d\n",child_pid, status);
    } else { // child
//        uint newmask = (1 << 19);
//        int oldmask = sigprocmask(newmask);
//        printf("child old mask after ignoring SIGCONT %d\n", oldmask);
//        newmask = (1 << 17);
//        oldmask = sigprocmask(newmask);
//        printf("child old mask after ignoring SIGSTOP %d\n", oldmask);
//        newmask = (1 << 9);
//        oldmask = sigprocmask(newmask);
//        printf("child old mask after ignoring SIGKILL %d\n", oldmask);
        sleep(15);

        for(int i = 1; i <= 50; i++){
            printf("counting: %d \n",i);
            sleep(1);
        }
    }
}

void signal_handler_user_sig_test(){
    printf("addr is %p\n", handler2);
    printf("addr is %p\n", handler3);
    printf("addr is %p\n", handler4);
    int child_pid = fork();
    if (child_pid < 0) {
        printf("fork failed\n");
    }
    else if (child_pid > 0) { // father
        sleep(10);
        int kill_res = -9;

        kill_res = kill(child_pid, 2);
        kill(child_pid, 2);
//        printf("first kill_res: %d\n",kill_res);
        kill_res = kill(child_pid, 3);
//        printf("sec kill_res: %d\n",kill_res);
        kill_res = kill(child_pid, 4);
//        printf("third kill_res: %d\n",kill_res);
        int status = kill_res;
        wait(&status);
        printf("Child PID: %d exit with status: %d\n",child_pid, status);
    } else { // child
        struct sigaction act;
        act.sigmask = 0;
        struct  sigaction oldact;
        act.sa_handler = handler2; // put the address of func handler
        sigaction(2,&act,&oldact);
        struct sigaction act2;
        act2.sa_handler = handler3; // SIG_IGN
        sigaction(3,&act2,&oldact);
        act.sa_handler = handler4;
        sigaction(4,&act,&oldact);
//        uint newmask = ((1 << 2) | (1 <<4) );
//        int oldmask = sigprocmask(newmask);
//        printf("child old mask %d\n", oldmask);
        sleep(20);
    }
}

int wait_sig = 0;

void test_handler(int signum) {
    wait_sig = 1;
    printf("Received sigtest\n");
}

void signal_test() {
    int pid;
    int testsig;
    testsig = 15;
    struct sigaction act = {test_handler, (uint)(1 << 29)};
    struct sigaction old;

    sigprocmask(0);
    sigaction(testsig, &act, &old);
    if ((pid = fork()) == 0) {
        while (!wait_sig)
            sleep(1);
        exit(0);
    }
    kill(pid, testsig);
    wait(&pid);
    printf("Finished testing signals\n");
}


void test_thread(){
    int tid = kthread_id();
    printf("Thread: %d is now running\n",tid);
//    for(int i = 0; i <= 20; i++){
//        printf("%d\n", i);
//    }
//    for(;;);
    kthread_exit(tid);
}
void thread_test(){
    int tid;
    int status;
    void* stack = malloc(MAX_STACK_SIZE);
    tid = kthread_create(test_thread, stack);
    printf("tid: %d after kthread_create\n",tid);
    sleep(10);
    kthread_join(tid,&status);
    tid = kthread_id();
    free(stack);
    printf("Finished testing threads, main thread id: %d, %d\n", tid, status);
}

int main(int argc, char *argv[]) {
// bsem test
//    struct counting_semaphore csem;
//    int retval;
//    int pid;
//
//    retval = csem_alloc(&csem,1);
//    if(retval==-1)
//    {
//        printf("failed csem alloc");
//        exit(-1);
//    }printf("bin1: %d, bin2:%d, initial value: %d\n",csem.binary_semaphore1, csem.binary_semaphore2, csem.value);
//    csem_down(&csem);
//    printf("1. Parent downing semaphore\n");
//    if((pid = fork()) == 0){
//        printf("2. Child downing semaphore\n");
//        csem_down(&csem);
//        printf("4. Child woke up\n");
//        exit(0);
//    }
//    sleep(5);
//    printf("3. Let the child wait on the semaphore...\n");
//    sleep(10);
//    csem_up(&csem);
//    csem_free(&csem);
//
//    wait(&pid);
//
//    printf("Finished bsem test, make sure that the order of the prints is alright. Meaning (1...2...3...4)\n");
//
// csem test
    int pid;
    int bid = bsem_alloc();
    bsem_down(bid);
    printf("1. Parent downing semaphore\n");
    if((pid = fork()) == 0){
        printf("2. Child downing semaphore\n");
        bsem_down(bid);
        printf("4. Child woke up\n");
        exit(0);
    }
    sleep(5);
    printf("3. Let the child wait on the semaphore...\n");
    sleep(10);
    bsem_up(bid);

    bsem_free(bid);
    wait(&pid);
    printf("Finished bsem test, make sure that the order of the prints is alright. Meaning (1...2...3...4)\n");
//
    exit(0);
}

