#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"
#include "kernel/syscall.h"
#include "kernel/param.h"
#include "Csemaphore.h"


#define BUFSZ 100
#define ITMNO 1000

int queue[BUFSZ];
int out;
int in;
int finishFlag;
struct counting_semaphore *empty;
struct counting_semaphore *full;
struct counting_semaphore *mutex;

int removeItem() {
    int retNum = queue[out];
    out = (out + 1) % BUFSZ;
    return retNum;
}

void addItem(int item) {
    queue[in] = item;
    in = (in + 1) % BUFSZ;
}


void producer(void *arg) {
    int i;
    for (i = 1; i <= ITMNO; i++) {
        csem_down(empty);
        csem_down(mutex);
        addItem(i);
        csem_up(mutex);
        csem_up(full);
    }
    printf("Producer TID: %d FINISHED\n",kthread_id());
    kthread_exit(kthread_id());
}

void consumer(void *arg) {
    while (!finishFlag) {
        csem_down(full);
        if (finishFlag)
            break;
        csem_down(mutex);
        int item = removeItem();
        finishFlag = (item == ITMNO);
        if (finishFlag)
            csem_free(full);    //Releases blocked threads on full (if any)
        csem_up(mutex);
        csem_up(empty);
        while (!item) {
            sleep(10);
        }
        printf("Thread %d WAKEUP form sleep\n");
    }
    printf("Consumer TID: %d FINISHED\n",kthread_id());
    kthread_exit(kthread_id());
}

int main(int argc, char *argv[]) {
    csem_alloc(empty, BUFSZ);
    csem_alloc(full, 0);
    csem_alloc(mutex, 1);
    int tid;
    int status;
    int ids[4];
    void *stacks[4];
    for (int i = 0; i < 3; i++) {
        void *stack = malloc(MAX_STACK_SIZE);
        tid = kthread_create(consumer, stack);
        ids[i] = tid;
        stacks[i] = stack;
        sleep(5);
    }
    void *stack = malloc(MAX_STACK_SIZE);
    tid = kthread_create(producer, stack);
    ids[3] = tid;
    stacks[3] = stack;
    for (int i = 0; i < 4; i++) {
        tid = ids[i];
        kthread_join(tid, &status);
        printf("joined with: %d exit status: %d\n", tid, status);
        free(stacks[i]);
    }
    csem_free(empty);
    csem_free(mutex);
    kthread_exit(0);  // exit process
    printf("got here : bad\n");
    exit(0);
}