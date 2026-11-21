#include <stdio.h>
#include <stdlib.h>

/* 1. Функція */
int my_function(void)
{
    return 42;
}

/* 2. Ініціалізована глобальна змінна */
int global_initialized = 10;

/* 3. Неініціалізована глобальна змінна */
int global_uninitialized;

/* 4. Ініціалізована статична змінна */
static int static_initialized = 20;

/* 5. Неініціалізована статична змінна */
static int static_uninitialized;

/* 6. Рядковий літерал */
const char *string_literal = "Computer Systems";

int main(void)
{
    /* 7. Локальна змінна */
    int local = 30;

    /* 8. Динамічне виділення */
    int *heap_allocation = malloc(sizeof(int));

    if (heap_allocation == NULL) {
        return 1;
    }

    *heap_allocation = 40;

    printf("Code/function:            %p\n", (void *)my_function);
    printf("Global initialized:       %p\n", (void *)&global_initialized);
    printf("Global uninitialized:     %p\n", (void *)&global_uninitialized);
    printf("Static initialized:       %p\n", (void *)&static_initialized);
    printf("Static uninitialized:     %p\n", (void *)&static_uninitialized);
    printf("String literal:           %p\n", (void *)string_literal);
    printf("Heap allocation:          %p\n", (void *)heap_allocation);
    printf("Stack/local:              %p\n", (void *)&local);

    free(heap_allocation);

    return 0;
}
