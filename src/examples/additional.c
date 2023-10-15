#include <stdio.h>
#include <syscall.h>
#include <stdlib.h>

int main(int argc, char* argv[]){ 
    int fib,maxi;
    fib=fibonacci(atoi(*(argv+1)));
    maxi=max_of_four_int(atoi(*(argv+1)),atoi(*(argv+2)),atoi(*(argv+3)),atoi(*(argv+4)));
    printf("%d %d\n",fib,maxi);
    return EXIT_SUCCESS;
}

