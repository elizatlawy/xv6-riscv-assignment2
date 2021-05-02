#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"
#include "kernel/syscall.h"

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

void handler(int x) {
    printf("Hey");

}

void sigaction_test() {
    struct sigaction act;
//    int sig_kill = 9;
    act.sa_handler = handler; // put the address of func handler
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
    act2.sa_handler = do_stuff2;
    printf("do_stuff2 address: %d\n", act2.sa_handler);
    printf("act2 sigmask: %d\n", act2.sigmask);
    struct sigaction old_act2;
    sigaction(15, &act, &old_act2);
    printf("old act2 sigmask %d\n", old_act2.sigmask);
    printf("old act2 address %d\n", old_act2.sa_handler);
}

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
        printf("exit wait -> child PID: %d killed with status: %d \n", child_pid,status);
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

int main(int argc, char *argv[]) {
//    fork();
//    printf("PID: %d\n", getpid());
//    sigprocmastk_test();
//    sigaction_test();
//    fork_test();
//    many_kills();
//    stopcont();
    exit(0);
}

