#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"
#include "kernel/syscall.h"


void sigprocmastk_test(){
    uint newmask = (1 << 9);
    uint oldmask = sigprocmask(newmask);
    printf("parent old mask%d\n", oldmask);
//    int pid = fork();
//    if(pid == 0){
//        uint newmask = (1 << 17);
//        uint oldmask = sigprocmask(newmask);
//        printf("child old mask%d\n", oldmask);
//    }

}

int main(int argc, char *argv[]) {

    sigprocmastk_test();

return 0;
}

