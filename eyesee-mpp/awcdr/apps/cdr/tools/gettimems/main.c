#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

int main(int argc, char *argv[])
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    fprintf(stderr, "%s time: %lds.%ldms\n", argv[1], tv.tv_sec, tv.tv_usec);

    return 0;
}
