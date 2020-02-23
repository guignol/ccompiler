
int printf();

void exit();

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

int count;

void assert(int expected, int actual, char *name) {
    count = count + 1;
    printf("%d: %s\n", count, name);
    if (expected != actual) {
        printf("=> %d expected but got %d\n", expected, actual);
        exit(1);
    }
}

/////////////////////////////////////////////////

//int scope_for_1() {
//    int a;
//    a = 1;
//    for (int i; a < 10; a++) {
//        i = 1;
//        a = i + a;
//    }
//    return a;
//}

// 3
int scoped_2() {
    int a = 3;
    int b = a;
    {
        bar(a);
        int a = b - 2;
        bar(a);
        if (a == b) {
            return a;
        }
    }
    return a;
}

// 1
int scoped_1() {
    int a_0; // 4
    int a_1; // 8
    int a_2; // 12
    int a_3; // 16
    a_0 = 33;
    {
        int a_0;
        a_0 = 5; // 20
        if (a_0 == 5) {
            return 1;
        }
    }
    return a_0;
}

/////////////////////////////////////////////////

char *moji_direct_return() {
    return "文字を返す関数\n";
}

int string_return() {
    printf(moji_direct_return());
    return 8;
}

/////////////////////////////////////////////////

char *moji_global;

void init_global() {
    moji_global = "moji_global";
}

int string_global() {
    init_global();
    printf(moji_global);
    return 4;
}

int string_literal_japanese() {
    printf("日本語ですね\n");
    return 9;
}

// 3
int string_literal_ascii() {
    char *moji;
    moji = "moji\ndesu\nne\n";
    printf(moji);
    return 3;
}

// 110
int string_literal_char_array_1() {
    char moji[4] = "moji"; // => 警告なし
    return sizeof(moji) / sizeof(*moji) + moji[2];
}

// 0
int string_literal_char_array_2() {
    char moji[10] = "moji"; // => 0終端
    return moji[4];
}

// 106
int string_literal_char_array_3() {
    char moji[3] = "moji"; // => 警告あり
    return moji[2];
}

// 4
int string_literal_char_array_4() {
    char moji[] = "moji"; // => 0終端
    return sizeof(moji) / sizeof(*moji);
}

// 5
int char_literal_1() {
    char a = 65;
    char b = 'B';
    char c = 'ac';
    // A, B, C
    printf("%c, %c, %c\n", a, b, c - 32);
    return 'F' - a;
}

// 6
int char_literal_2() {
    char msg[4] = {'f', 'o', 'o', '_'};
    // foo_ == foo_
    printf("foo_ == %.*s\n", 4, msg);
    return sizeof(msg) + 2;
}

// 7
int char_literal_3() {
    char msg[4] = {'f', 'qo', '\o', 'qwe\0'};
    // foo == foo
    printf("foo == %s\n", msg);
    return sizeof(msg) + 3;
}

// 9
int char_literal_4() {
    char msg[] = {'b', 'o', 'o', '\0'};
    // boo == boo
    printf("boo == %s\n", msg);
    return sizeof(msg) + 5;
}

/////////////////////////////////////////////////

// 13
int char_array_and_pointer_1() {
    char a[4];
    char (*b)[4];
    b = &a;
    (*b)[3] = 13;
    return (*b)[3];
}

// 8
int char_array_and_pointer_2() {
    char a[2][3];
    char (*b)[3];
    b = &a;
    a[1][2] = 5;
    return a[1][2] + 3;
}

// 24
int char_array_and_pointer_3() {
    char *chapos[2] = {"abc", "def"};
    printf("%s%s\n", chapos[0], chapos[1]);
    return sizeof(chapos) /* 16 = 2(array size) * 8(pointer size) */
           + sizeof(chapos[0]) /* 8(pointer size) */;
}

// 4
int char_array_and_pointer_4() {
    char *chapos[] = {"ab", "cd", "ef", "gh"};
    int array_size = sizeof(chapos) /* 32 = 4(array size) * 8(pointer size) */
                     / sizeof(*chapos);  /* 先頭要素 8(pointer size) */
    return array_size;
}

// 3
int char_array_and_pointer_5() {
//    char charata[2][3][4] = {
//            {
//                    {1, 2, 3, 4},
//                    {1, 2},
//                    {1, 2},
//            },
//            {
//                    {1, 2},
//                    {1, 2},
//                    {1, 2},
//            },
//    };
    // TODO [2][3]だとprintfが変になるはず
//    char charara[2][4] = {"abc", "def"};
    /*
     * 0: pointer  (1 * 8 byte)
     * 1: pointer
     *
     * ↑ではなく
     * ↓型と現実
     *
     * 0: char char char (3 * 1 byte)
     * 1: char char char
     *     *
     * ↓ x86-64 gcc 9.1以降（これに近い方式を採用する）
     *
     *   sub rsp, 16
     *   mov WORD PTR [rbp-6], 25185
     *   mov BYTE PTR [rbp-4], 99
     *   mov WORD PTR [rbp-3], 25956
     *   mov BYTE PTR [rbp-1], 102
     *
     * ↓ x86-64 gcc 4.7.4以前
     *
     *   sub rsp, 16
     *   mov WORD PTR [rbp-16], 25185
     *   mov BYTE PTR [rbp-14], 99
     *   mov WORD PTR [rbp-13], 25956
     *   mov BYTE PTR [rbp-11], 102
     *
     * ↓ その他のgcc
     *
     * .LC0:
     *   .ascii "abc"
     *   .ascii "def"
     * ---------------
     *   sub rsp, 16
     *   mov eax, DWORD PTR .LC0[rip]
     *   mov DWORD PTR [rbp-6], eax
     *   movzx eax, WORD PTR .LC0[rip+4]
     *   mov WORD PTR [rbp-2], ax
     *
     */
//    printf("%s%s\n", charara[0], charara[1]);
    return 3;
}

// 3
int char_calculate_array() {
    char x[3];
    x[0] = -1;
    x[1] = 2;
    int y;
    y = 4;
    return x[0] + y;
}

/////////////////////////////////////////////////

int *a;
int *b;
int aa;

// 5
int global_variable_1() {
    int a; // 型の異なるローカル変数
    a = 3;
    b = &a;
    *b = 2 + a;
    return a;
}

// 9
int global_variable_2() {
    aa = 1;
    b = &aa;
    *b = 8 + aa;
    return aa;
}

// 3
int init() {
    aa = 6;
}

int global_variable_3() {
    int b;
    init();
    b = aa;
    int aa;
    aa = 3;
    return b - aa;
}

// 3
int global_variable_4() {
    aa = 3;
    int b;
    b = 6;
    return b - aa;
}

/////////////////////////////////////////////////

int write_stack_4() {
    int x[4] = {0, 1, 2, 3};
    return x[3];
}

int zero_stack_4() {
    // 4つめはゼロ埋めされる
    int x[4] = {0, 1, 2,};
    return x[3];
}

// 0
int array_initialize_1() {
    write_stack_4();
    return zero_stack_4();
}

// 2
int array_initialize_2() {
    int array[] = {0, 1, 2, 3};
    return array[2];
}

// 4
int array_initialize_3() {
    int a = 3;
    int b = 1;
    int *array[5] = {0, a, &a, &b};
    return *array[2] + *array[3];
}

//
int array_initialize_4() {
    // TODO
//    int array[3][2] = {{3, 3}, {3, 3}, {3, 3}};

    return 1;
}

/////////////////////////////////////////////////

// 8
int array_and_pointer_1() {
    int a[2][3];
    int *b;
    int (*c)[4];
    b = a;
    c = a;
    a[1][2] = 5;
    return a[1][2] + 3;
}

// 8 
int array_and_pointer_2() {
    int a[2][3];
    int (*b)[3];
    b = &a;
    a[1][2] = 5;
    return a[1][2] + 3;
}

// 8 
int array_and_pointer_3() {
    int a[2][3];
    int (*b)[3];
    b = a;
    a[1][2] = 5;
    return a[1][2] + 3;
}

// 8 
int array_and_pointer_4() {
    int a[2];
    a[0] = 1;
    a[1] = 2;
    int (*b)[2];
    b = &a;
    b = a;
    (*b)[0] = 5;
    (*b)[1] = 3;
    return a[0] + a[1];
}

// 12 
int array_and_pointer_5() {
    int a[2][3];
    int *b;
    int c;
    c = 1;
    b = &c;
    (*b)[a][2] = 12;
    return ((*b)[a])[2];
}

// 12 
int array_and_pointer_6() {
    int a[2][3];
    int b[5];
    b[4] = 1;
    (a[0])[b[4]] = 12;
    return (a[0])[1];
}

// 12 
int array_and_pointer_7() {
    int a[2][3][4];
    int one;
    one = 1;
    a[0][one][2] = 7;
    one[a][2][3] = 5;
    return a[0][one][2] + a[one][2][3];
}

// 12 
int array_and_pointer_8() {
    int a[2][3];
    a[0][0] = 11;
    a[0][1] = 12;
    a[0][2] = 13;
    a[1][0] = 21;
    a[1][1] = 22;
    a[1][2] = 23;
    return a[0][1];
}

// 8 
int array_and_pointer_9() {
    int a[2];
    int b;
    b = sizeof(a);
    return b;
}

/////////////////////////////////////////////////

// 24
int array_and_pointer_10() {
    int a[2][3];
    int b;
    b = sizeof(a);
    return b;
}

// 3
int array_and_pointer_11() {
    int a[2];
    int *b;
    int c;
    c = 3;
    b = &c;
    a[0] = *b;
    b = &a;
    // &a = &c; // TODO これはできない
    return *b;
}

// 3
int array_and_pointer_12() {
    int a;
    int *b;
    int **c;
    int ***d;
    a = 1;
    b = &a;
    c = &b;
    d = &c;
    a = ***d + 2;
    return a;
}

// 3
int array_and_pointer_13() {
    // ポインタへのポインタの配列
    int **p[4];
    int a;
    int *b;
    int **c;
    a = 1;
    b = &a;
    c = &b;
    p[0] = c;
    a = *(*(p[0])) + 2;
    return **p[0];  // 3
}

// 3
int array_and_pointer_14() {
    int a[2];
    0[a] = 1;
    1[a] = 2;
    int *p;
    p = a;
    int maji;
    maji = 0[p]; // ポインタにも[]使える
    return maji + 1[a];  // 3
}

// 3
int array_and_pointer_15() {
    int a[5] = {1, 2, 3, 4, 5};
    int maji;
    maji = a[1 + 3];
    return maji - a[1];  // 3
}

// 3
int array_and_pointer_16() {
    int a[2];
    a[0] = 1;
    a[1] = 2;
    int *p;
    p = a;
    int maji;
    maji = p[0]; // ポインタにも[]使える
    return maji + a[1];  // 3
}

// 3
int array_and_pointer_17() {
    int *a[2];
    int b;
    int *c;
    int *d;
    b = 3;
    c = &b;
    *a = c;
    d = *a;
    return *d;
}

// 3
int array_and_pointer_18() {
    int a[2];
    *a = 3;
    return *a;
}

// 3
int array_and_pointer_19() {
    int a[2];
    *a = 1;
    *(a + 1) = 2;
    int *p;
    p = a;
    return *p + *(p + 1);  // 3
}

/////////////////////////////////////////////////

// 4
int sizeof_1() {
    int x;
    return sizeof(x);
}

// 8
int sizeof_2() {
    int *y;
    return sizeof(y);
}

// 4
int sizeof_3() {
    int x;
    return sizeof(x + 3);
}

// 8
int sizeof_4() {
    int *y;
    return sizeof(y + 3);
}

// 4
int sizeof_5() {
    int *y;
    return sizeof(*y);
}

// 4
int sizeof_6() {
    // sizeofに渡す式は何でもよい
    return sizeof(1);
}

// 4
int sizeof_7() {
    // このコンパイラでは
    // sizeofの結果はint型なのでsizeof(int)と同じ
    // 実際のCでは
    // sizeofの結果はsize_t型（8バイト）になる
    return sizeof(sizeof(1));
}

char chaa(char c) {
    return c + 1;
}

// 1
int sizeof_8() {
    char c;
    c = sizeof(c);
    int i;
    i = chaa(c);
    return i - c;
}

/////////////////////////////////////////////////

// 3
int pointer_and_calculate_1() {
//    int array_4[4] = {0, 1, 2, 3, 4};
    int array_4[4] = {0, 1, 2, 3,};

    int *p;
    int *q;
    p = array_4;

    // foo(*p);
    q = p + 3;
    return q - p;
}

// 2
int pointer_and_calculate_2() {
    int array_4[5] = {0, 1, 2, 3,};
    array_4[0] = 0;
    array_4[1] = 1;
    array_4[2] = 2;
    array_4[3] = 3;

    int *p;
    p = array_4;

    bar(*p);
    p = p + 1;
    p = 1 + p;
    return *p;
}

// 3
int pointer_and_calculate_3() {
    int x;
    int i;
    int y;
    x = 3;
    y = 5;
    // ポインタ演算では8bytesずつ動くので、
    // 4bytesずつ並んでいるintの2つ分スタックポインタを動かす
    i = *(&y + 2); // ポインタ演算？（関数フレームの実装に依存）
    return i;
}

// 4
int pointer_and_calculate_4() {
    int x;
    int *y;
    x = 3;
    y = &x;
    *y = 4;
    return x;
}

// 3
int pointer_and_calculate_5() {
    int x;
    x = 3;
    return *(&x);
}

/////////////////////////////////////////////////

int fibonacci(int n) {
    if (n == 0) {
        return 1;
    } else if (n == 1) {
        return 1;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

int add_1(int a, int b) {
    hoge(a, b);
    return a + b;
}

int add_2(int a, int b) { return a + b; }

int salut_1() {
    int a;
    int b;
    a = 1;
    b = 12;
    return 13;
}

int salut_2() { return 13; }

// 13
int function_1() { return add_1(1, 12); }

// 13
int function_2() { return add_2(1, 12); }

// 13
int function_3() { return salut_1(); }

// 13
int function_4() { return salut_2(); }

// 13
int function_5() { return bar(13); }

// 37
int function_6() {
    int a;
    int b;
    a = 1983;
    b = 2020;
    if ((b - a) == 37) return 37; else return 36;
}

// 12
int function_7() {
    int a;
    a = 13;
    if (a == 0) return 3; else return 12;
}

// 12
int function_8() { if (0) return 3; else return 12; }

// 1
int function_9() {
    int i;
    for (i = 0; i < 10; i = i + 1) {
        hoge(i, fibonacci(i));
    }
    return 1;
}

/////////////////////////////////////////////////

// 13
int function_10() {
    printf("moji: %i", 13);
    return 13;
}

// 13
int function_11() {
    int a;
    a = 13;
    printf("moji: %i", 13);
    return a;
}

// 12
int function_12() {
    int b;
    b = 1;
    int a;
    a = foo() + b;
    return a;
}

// 11
int function_13() {
    return foo();
}

// 12
int function_14() {
    int a;
    return a = foo() + 1;
}

// 12
int function_15() {
    int a;
    return a = bar(11) + 1;
}

// 12
int function_16() {
    int a;
    int b;
    b = 11;
    return a = bar(b) + 1;
}

// 12
int function_17() {
    int a;
    return a = hoge(11, 1);
}

// 12
int function_18() {
    int a;
    int b;
    a = 1;
    b = 11;
    return hoge(b, a);
}

// 12
int function_19() {
    return hoge(11, 1);
}

/////////////////////////////////////////////////

// 10
int block_1() {
    int a;
    a = 0;
    int b;
    b = 1;
    int i;
    for (i = 0; i < 100; i = i + 1) {
        a = a + 1;
        b = a + 1;
        if (a == 10) return a;
    }
    return b - 1;
}

// 11
int block_2() {
    int a;
    a = 3;
    int b;
    b = 2;
    int c;
    c = 6;
    return a + b + c;
}

// 11
int block_3() {
    int a;
    int b;
    int c;
    {
        a = 3;
        b = 2;
        c = 6;
        return a + b + c;
    }
}

// 3
int block_4() {
    {
        int aa;
        int b;
        aa = 3;
        printf("%d", aa);
        { b = 2; }
        return aa;
    }
}

// 10
int block_5() {
    int a;
    int b;
    int i;
    a = 0;
    b = 1;
    for (i = 0; i < 10; i = i + 1) {
        a = a + 1;
        b = a + 1;
    }
    return b - 1;
}

// 10
int block_6() {
    int a;
    int b;
    int i;
    a = 0;
    b = 1;
    for (i = 0; i < 10; i = i + 1) {
        a = a + 1;
        b = a + 1;
    }
    return a;
}

// 10
int block_7() {
    int value;
    int i;
    value = 0;
    for (i = 0; i < 10; i = i + 1) { value = value + 1; }
    return value;
}

// 6
int block_8() {
    {
        int a;
        int b;
        int c;
        a = 3;
        b = 2;
        return c = 6;
    }
}

// 6
int block_9() {
    {
        int a;
        int b;
        int c;
        a = 3;
        b = 2;
        return c = 6;
        return a + b + c;
    }
}

/////////////////////////////////////////////////

int assert_others() {
    assert(99, ({
        int a;
        a = 0;
        int i;
        for (i = 0; i < 10; i = i + 1) a = a + 1;
        99;
    }), "");
    assert(10, ({
        int a;
        a = 0;
        int i;
        for (i = 0; i < 10; i = i + 1) a = a + 1;
        a;
    }), "");
    assert(10, ({
        int a;
        a = 0;
        while (a < 10) a = a + 1;
        a;
    }), "");

    assert(25, ({
        int a_3;
        int _loc;
        a_3 = 12;
        _loc = 3;
        a_3 * _loc - 11;
    }), "");
    assert(5, ({
        int aaa;
        aaa = 3;
        aaa + 2;
    }), "");
    assert(5, ({
        int b29;
        int aaa;
        aaa = 3;
        b29 = 2;
        b29 + aaa;
    }), "");
    assert(5, ({
        int O0;
        O0 = 3;
        O0 = 2;
        O0 + 3;
    }), "");

    assert(5, ({
        int a;
        a = 3;
        a + 2;
    }), "");
    assert(3, ({
        int a;
        a = 3;
        a;
    }), "");
    assert(5, ({
        int a;
        int z;
        a = 3;
        z = 2;
        a + z;
    }), "");

    assert(0, ({ 0; }), "");
    assert(42, ({ 42; }), "");
    assert(42, ({ 40 + 2; }), "");
    assert(0, ({ 100 - 100; }), "");
    assert(10, ({ 100 - 100 + 10; }), "");
    assert(111, ({ 100 + 10 + 1; }), "");
    assert(100, ({ 100 * 10 / 10; }), "");
    assert(101, ({ 100 + 10 / 10; }), "");
    assert(0, ({ 10 * -1 + 10; }), "");
    assert(90, ({ 100 + -1 * 10; }), "");

    assert(0, ({ 0 == 1; }), "");
    assert(1, ({ 42 == 42; }), "");
    assert(1, ({ 0 != 1; }), "");
    assert(0, ({ 42 != 42; }), "");
    assert(1, ({ 42 != 42 + 1; }), "");
    assert(1, ({ 2 + 42 != 42; }), "");
    assert(1, ({ (2 + 42) != 42; }), "");

    assert(1, ({ 0 < 1; }), "");
    assert(0, ({ 1 < 1; }), "");
    assert(0, ({ 2 < 1; }), "");
    assert(1, ({ 0 <= 1; }), "");
    assert(1, ({ 1 <= 1; }), "");
    assert(0, ({ 2 <= 1; }), "");

    assert(1, ({ 1 > 0; }), "");
    assert(0, ({ 1 > 1; }), "");
    assert(0, ({ 1 > 2; }), "");
    assert(1, ({ 1 >= 0; }), "");
    assert(1, ({ 1 >= 1; }), "");
    assert(0, ({ 1 >= 2; }), "");
}

/////////////////////////////////////////////////

int main() {

    // TODO
//    assert(9, for_init_scope_1(), "scope_for_1");
    assert(3, scoped_2(), "scoped_2");
    assert(1, scoped_1(), "scoped_1");

    // TODO ポインタを返す関数のテストを追加
    assert(8, string_return(), "string_return");

    assert(4, string_global(), "string_global");
    assert(9, string_literal_japanese(), "string_literal_japanese");
    assert(3, string_literal_ascii(), "string_literal_ascii");
    assert(110, string_literal_char_array_1(), "string_literal_char_array_1");
    assert(0, string_literal_char_array_2(), "string_literal_char_array_2");
    assert(106, string_literal_char_array_3(), "string_literal_char_array_3");
    assert(4, string_literal_char_array_4(), "string_literal_char_array_4");

    assert(5, char_literal_1(), "char_literal_1");
    assert(6, char_literal_2(), "char_literal_2");
    assert(7, char_literal_3(), "char_literal_3");
    assert(9, char_literal_4(), "char_literal_4");

    assert(13, char_array_and_pointer_1(), "char_array_and_pointer_1");
    assert(8, char_array_and_pointer_2(), "char_array_and_pointer_2");
    assert(24, char_array_and_pointer_3(), "char_array_and_pointer_3");
    assert(4, char_array_and_pointer_4(), "char_array_and_pointer_4");
//    assert(3, char_array_and_pointer_5(), "char_array_and_pointer_5");
    assert(3, char_calculate_array(), "char_calculate_array");

    assert(5, global_variable_1(), "global_variable_1");
    assert(9, global_variable_2(), "global_variable_2");
    assert(3, global_variable_3(), "global_variable_3");
    assert(3, global_variable_4(), "global_variable_4");

    assert(0, array_initialize_1(), "array_initialize_1");
    assert(2, array_initialize_2(), "array_initialize_2");
    assert(4, array_initialize_3(), "array_initialize_3");

    assert(8, array_and_pointer_1(), "array_and_pointer_1");
    assert(8, array_and_pointer_2(), "array_and_pointer_2");
    assert(8, array_and_pointer_3(), "array_and_pointer_3");
    assert(8, array_and_pointer_4(), "array_and_pointer_4");
    assert(12, array_and_pointer_5(), "array_and_pointer_5");
    assert(12, array_and_pointer_6(), "array_and_pointer_6");
    assert(12, array_and_pointer_7(), "array_and_pointer_7");
    assert(12, array_and_pointer_8(), "array_and_pointer_8");
    assert(8, array_and_pointer_9(), "array_and_pointer_9");

    assert(24, array_and_pointer_10(), "array_and_pointer_10");
    assert(3, array_and_pointer_11(), "array_and_pointer_11");
    assert(3, array_and_pointer_12(), "array_and_pointer_12");
    assert(3, array_and_pointer_13(), "array_and_pointer_13");
    assert(3, array_and_pointer_14(), "array_and_pointer_14");
    assert(3, array_and_pointer_15(), "array_and_pointer_15");
    assert(3, array_and_pointer_16(), "array_and_pointer_16");
    assert(3, array_and_pointer_17(), "array_and_pointer_17");
    assert(3, array_and_pointer_18(), "array_and_pointer_18");
    assert(3, array_and_pointer_19(), "array_and_pointer_19");

    assert(4, sizeof_1(), "sizeof_1");
    assert(8, sizeof_2(), "sizeof_2");
    assert(4, sizeof_3(), "sizeof_3");
    assert(8, sizeof_4(), "sizeof_4");
    assert(4, sizeof_5(), "sizeof_5");
    assert(4, sizeof_6(), "sizeof_6");
    assert(4, sizeof_7(), "sizeof_7"); // ★
    assert(1, sizeof_8(), "sizeof_8");

    assert(3, pointer_and_calculate_1(), "pointer_and_calculate_1");
    assert(2, pointer_and_calculate_2(), "pointer_and_calculate_2");
    assert(3, pointer_and_calculate_3(), "pointer_and_calculate_3"); // ★
    assert(4, pointer_and_calculate_4(), "pointer_and_calculate_4");
    assert(3, pointer_and_calculate_5(), "pointer_and_calculate_5");

    assert(13, function_1(), "function_1");
    assert(13, function_2(), "function_2");
    assert(13, function_3(), "function_3");
    assert(13, function_4(), "function_4");
    assert(13, function_5(), "function_5");
    assert(37, function_6(), "function_6");
    assert(12, function_7(), "function_7");
    assert(12, function_8(), "function_8");
    assert(1, function_9(), "function_9");

    assert(13, function_10(), "function_10");
    assert(13, function_11(), "function_11");
    assert(12, function_12(), "function_12");
    assert(11, function_13(), "function_13");
    assert(12, function_14(), "function_14");
    assert(12, function_15(), "function_15");
    assert(12, function_16(), "function_16");
    assert(12, function_17(), "function_17");
    assert(12, function_18(), "function_18");
    assert(12, function_19(), "function_19");

    assert(10, block_1(), "block_1");
    assert(11, block_2(), "block_2");
    assert(11, block_3(), "block_3");
    assert(3, block_4(), "block_4");
    assert(10, block_5(), "block_5");
    assert(10, block_6(), "block_6");
    assert(10, block_7(), "block_7");
    assert(6, block_8(), "block_8");
    assert(6, block_9(), "block_9");

    assert_others();

    printf("=========== ============= ============\n");

    return 0;
}