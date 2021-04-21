#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"
#include "kernel/syscall.h"


void sigprocmastk_test(){
    uint newmask = (1 << 9);
    int oldmask = sigprocmask(newmask);
    printf("parent old mask %d\n", oldmask);
    int pid = fork();
    if(pid == 0){
        uint newmask = (1 << 17);
        uint oldmask = sigprocmask(newmask);
        printf("child old mask %d\n", oldmask);
    }
}

void handler(int x){
    printf("Hey");

}

void sigaction_test(){
    struct sigaction act;
//    int sig_kill = 9;
//    act.sa_handler = handler;
    act.sa_handler = (void*)9;
    act.sigmask = (1 << 9);
    printf("act address: %p\n", act);
    printf("act sigmask: %d\n", act.sigmask);
    struct sigaction old_act;
    sigaction(15,&act,&old_act);
    printf("old act sigmask %d\n", old_act.sigmask);
    printf("old act sigmask %d\n", old_act.sa_handler);
}

int main(int argc, char *argv[]) {
    sigaction_test();
//    sigprocmastk_test();


    exit(0);
}

