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