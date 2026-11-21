#include <stdio.h>

int initialized = 123;
int large_array[100000];

static int static_initialized = 456;
static int static_uninitialized;

const char message[] = "Computer Systems";

int main(void)
{
    printf("%d %d %d %d %s\n",
           initialized,
           large_array[0],
           static_initialized,
           static_uninitialized,
           message);

    return 0;
}
