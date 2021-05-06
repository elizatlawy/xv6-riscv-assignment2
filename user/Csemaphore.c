#include "Csemaphore.h"
#include "kernel/syscall.h"
#include "kernel/types.h"
#include "user.h"
#include "kernel/fcntl.h"
#include "kernel/syscall.h"




int csem_alloc(struct counting_semaphore *sem, int initial_value){
    int binary_semaphore1 = bsem_alloc();
    int binary_semaphore2 = bsem_alloc();
    if (binary_semaphore1 == -1 || binary_semaphore2 == -1)
        return -1; // There are no binary semaphores available to implement a counting semaphore
    if (initial_value == 0)
        bsem_down(sem->binary_semaphore2); // binary_semaphore2 initial value = min(1, initial_value);
    sem->binary_semaphore1 = binary_semaphore1;
    sem->binary_semaphore2 = binary_semaphore2;
    sem->value = initial_value;

    return 0;
}

void csem_free(struct counting_semaphore *sem){
    bsem_free(sem->binary_semaphore1);
    bsem_free(sem->binary_semaphore2);
    free(sem);
}

void csem_down(struct counting_semaphore *sem){
    bsem_down(sem->binary_semaphore2);
    bsem_down(sem->binary_semaphore1);
    sem->value--;
    if (sem->value > 0)
        bsem_up(sem->binary_semaphore2);
    bsem_up(sem->binary_semaphore1);
}

void csem_up(struct counting_semaphore *sem){

    bsem_down(sem->binary_semaphore1);
    sem->value++;
    if (sem->value == 1){
        printf("hey\n");
        bsem_up(sem->binary_semaphore2);
    }
    printf("hey2\n");
    bsem_up(sem->binary_semaphore1);
    printf("hey3\n");
}