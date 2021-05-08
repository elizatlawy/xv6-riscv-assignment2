#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"
#include "kernel/syscall.h"
#include "kernel/param.h"
#include "Csemaphore.h"


void sigprocmastk_test(char *s) {
    uint newmask = (1 << 9);
    int oldmask = sigprocmask(newmask);
    printf("parent old mask %d\n", oldmask);
    int pid = fork();
    if (pid == 0) {
        uint newmask = (1 << 17);
        uint oldmask = sigprocmask(newmask);
        printf("child old mask %d\n", oldmask);
    }
    exit(0);
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

int flag = 0;

void
do_stuff(int a) {
    flag = 1;
}

void sigaction_test(char *s) {
    struct sigaction act;
//    int sig_kill = 9;
    act.sa_handler = handler2; // put the address of func handler
//    act.sa_handler = (void*)9;
    act.sigmask = (1 << 9);
    printf("handler address: %d\n", act.sa_handler);
    printf("act sigmask: %d\n", act.sigmask);
    struct sigaction old_act;
    sigaction(15, &act, &old_act);
    printf("old act sigmask %d\n", old_act.sigmask);
    printf("old act sigmask %d\n", old_act.sa_handler);
    struct sigaction act2;
    act2.sigmask = (1 << 9);
    act2.sa_handler = do_stuff;
    printf("do_stuff2 address: %d\n", act2.sa_handler);
    printf("act2 sigmask: %d\n", act2.sigmask);
    struct sigaction old_act2;
    sigaction(15, &act, &old_act2);
    printf("old act2 sigmask %d\n", old_act2.sigmask);
    printf("old act2 address %d\n", old_act2.sa_handler);
}

void sigstop_sigcont_test(){
    int child_pid = fork();
    if (child_pid < 0) {
        printf("fork failed\n");
    }
    else if (child_pid > 0) { // father
        sleep(20);
        int kill_res = -9;
        sleep(10);
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
        sleep(15);
        for(int i = 1; i <= 50; i++){
            printf("counting: %d \n",i);
            sleep(1);
        }
    }
}
void sigstop_sigcont_sigkill_with_mask_test(){
    int child_pid = fork();
    if (child_pid < 0) {
        printf("fork failed\n");
    }
    else if (child_pid > 0) { // father
        sleep(20);
        int kill_res = -9;
        sleep(10);
        kill_res = kill(child_pid, 17); // SIGSTOP
        printf(" SIGSTOP sent to child: %d with res: %d\n",child_pid,kill_res);
        sleep(10);
        kill_res = kill(child_pid, 9); // SIGKILL
        printf(" SIGKILL sent to child: %d with res: %d\n",child_pid,kill_res);
        int status;
        wait(&status);
        printf("Child PID: %d exit with status: %d\n",child_pid, status);
    } else { // child
        uint newmask = (1 << 19);
        int oldmask = sigprocmask(newmask);
        printf("child old mask after ignoring SIGCONT %d\n", oldmask);
        newmask = (1 << 17);
        oldmask = sigprocmask(newmask);
        printf("child old mask after ignoring SIGSTOP %d\n", oldmask);
        newmask = (1 << 9);
        oldmask = sigprocmask(newmask);
        printf("child old mask after ignoring SIGKILL %d\n", oldmask);
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
        printf("first kill_res: %d\n",kill_res);
        kill_res = kill(child_pid, 3);
        printf("sec kill_res: %d\n",kill_res);
        kill_res = kill(child_pid, 4);
        printf("third kill_res: %d\n",kill_res);
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
        sleep(10);
        exit(0);
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

//
// use sbrk() to count how many free physical memory pages there are.
// touches the pages to force allocation.
// because out of memory with lazy allocation results in the process
// taking a fault and being killed, fork and report back.
//
int
countfree()
{
    int fds[2];

    if(pipe(fds) < 0){
        printf("pipe() failed in countfree()\n");
        exit(1);
    }

    int pid = fork();

    if(pid < 0){
        printf("fork failed in countfree()\n");
        exit(1);
    }

    if(pid == 0){
        close(fds[0]);

        while(1){
            uint64 a = (uint64) sbrk(4096);
            if(a == 0xffffffffffffffff){
                break;
            }

            // modify the memory to make sure it's really allocated.
            *(char *)(a + 4096 - 1) = 1;

            // report back one more page.
            if(write(fds[1], "x", 1) != 1){
                printf("write() failed in countfree()\n");
                exit(1);
            }
        }

        exit(0);
    }

    close(fds[1]);

    int n = 0;
    while(1){
        char c;
        int cc = read(fds[0], &c, 1);
        if(cc < 0){
            printf("read() failed in countfree()\n");
            exit(1);
        }
        if(cc == 0)
            break;
        n += 1;
    }

    close(fds[0]);
    wait((int*)0);

    return n;
}

// run each test in its own process. run returns 1 if child's exit()
// indicates success.
int
run(void f(char *), char *s) {
    int pid;
    int xstatus;

    printf("test %s: ", s);
    if((pid = fork()) < 0) {
        printf("runtest: fork error\n");
        exit(1);
    }
    if(pid == 0) {
        f(s);
        exit(0);
    } else {
        wait(&xstatus);
        if(xstatus != 0)
            printf("FAILED\n");
        else
            printf("OK\n");
        return xstatus == 0;
    }
}

int
main(int argc, char *argv[]) {
    int continuous = 0;
    char *justone = 0;

    if (argc == 2 && strcmp(argv[1], "-c") == 0) {
        continuous = 1;
    } else if (argc == 2 && strcmp(argv[1], "-C") == 0) {
        continuous = 2;
    } else if (argc == 2 && argv[1][0] != '-') {
        justone = argv[1];
    } else if (argc > 1) {
        printf("Usage: usertests [-c] [testname]\n");
        exit(1);
    }

    struct test {
        void (*f)(char *);

        char *s;
    } tests[] = {
            {sigprocmastk_test,                       "sigprocmastk test"},
            {sigaction_test,                          "sigaction test"},
            {signal_test,                             "signal test"},
            {signal_handler_user_sig_test,            "signal handler user sig test"},
            {sigstop_sigcont_test,                    "sigstop sigcont test"},
            {sigstop_sigcont_sigkill_with_mask_test, "sigstop sigcont_sigkill with_mask test"},
            {0,                                       0},
    };

    if (continuous) {
        printf("continuous usertests starting\n");
        while (1) {
            int fail = 0;
            int free0 = countfree();
            for (struct test *t = tests; t->s != 0; t++) {
                if (!run(t->f, t->s)) {
                    fail = 1;
                    break;
                }
            }
            if (fail) {
                printf("SOME TESTS FAILED\n");
                if (continuous != 2)
                    exit(1);
            }
            int free1 = countfree();
            if (free1 < free0) {
                printf("FAILED -- lost %d free pages\n", free0 - free1);
                if (continuous != 2)
                    exit(1);
            }
        }
    }
    printf("signal starting\n");
    int free0 = countfree();
    int free1 = 0;
    int fail = 0;
    for (struct test *t = tests; t->s != 0; t++) {
        if ((justone == 0) || strcmp(t->s, justone) == 0) {
            if (!run(t->f, t->s))
                fail = 1;
        }
    }

    if (fail) {
        printf("SOME TESTS FAILED\n");
        exit(1);
    } else if ((free1 = countfree()) < free0) {
        printf("FAILED -- lost some free pages %d (out of %d)\n", free1, free0);
        exit(1);
    } else {
        printf("ALL TESTS PASSED\n");
        exit(0);
    }
}