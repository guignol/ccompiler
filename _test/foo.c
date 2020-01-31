#include <stdio.h>

int hoge(int x, int y) {
    printf("--------------hoge: %i, %i\n", x, y);
    return x + y;
}

int bar(int v) {
    printf("--------------bar: %i\n", v);
    return v;
}

int foo() {
    return bar(11);
}

// alloc4(&p, 1, 2, 4, 8);
int alloc_array_4(int **p, int a, int b, int c, int d) {
    int array[4] = {a, b, c, d};
    *p = array;
    return **p;
}