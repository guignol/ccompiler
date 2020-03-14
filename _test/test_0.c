
int printf();

void exit();

void exit(int status);

void exit(int status);

void exit();

int hoge(int, int);

int hoge(int x, int y);
//int hoge(int a, int b, int c);

int hoge(int x, int y) {
    printf("--------------hoge: %i, %i\n", x, y);
    return x + y;
}

int bar();

int bar(int v) {
    printf("--------------bar: %i\n", v);
    return v;
}

int foo(void);

int foo();

int foo(void);

int foo() {
    return bar(11);
}

int count;

void assert(char *, int expected, int);

void assert(char *name, int expected, int actual) {
    count = count + 1;
    printf("%d: %s\n", count, name);
    if (expected != actual) {
        printf("=> %d expected but got %d\n", expected, actual);
        exit(1);
    }
}

/////////////////////////////////////////////////

struct Box {
    int a;
    int b;
//    char a; // TODO エラーメッセージが微妙
};
struct Box box;

struct A;
// グローバル変数の宣言は前後のどこかで定義されてればOK（配列はダメ）
// ローカル変数はグローバルな構造体も定義が前方にある必要がある
// 関数の返り値や引数はローカル変数と同じ扱い
struct A a_s;
struct A *a_s_p;
struct A {
    int a;
};
struct Box a_a[3];
struct Box a_0;
struct Box a_1;
struct Box a_2;

// 13
int use_struct_1() {
//    count.a = 1;
//    box. = 1;
//    box.c = 1;
    box.a = 13;
    a_s.a = box.a;
    return a_s.a;
}

// 7
int use_struct_2() {
    a_0.b = 0;
    a_1.b = 1;
    a_2.b = 2;
    a_a[0] = a_0;
    a_a[1] = a_1;
    a_a[2] = a_2;
    int sum = 0;
    for (int i = 0; i < sizeof(a_a) / sizeof(int); i = i + 1) {
//        struct Box b = a_a[i];
//        sum = sum + b.b;
////        sum = sum + a_a[i].b;
    }
//    return sum + sizeof(struct A);
    a_0 = a_a[2];
    return a_0.b * 2 + a_1.b + a_2.b;
}

//int use_struct_3() {
//    // ローカルで宣言した時点でグローバルの同名構造体は隠蔽される
//    struct A;
////    struct A a;
//    // ローカル変数は前方で定義されている必要があるが、
//    // ポインタならスタックサイズが決まるからか前方に宣言があればOK
//    // たぶん意味ないけど後方に定義が無くても大丈夫
//    // ただし、サイズ情報が必要なこと（ポインタ演算、配列の宣言）はできない
//    struct A *a;
//    a = 0;
////    a++;
////    struct A {
////        int a;
////    };
////    struct A array[3];
//}


/////////////////////////////////////////////////

// 11
int scope_for_1() {
    int a;
    a = 1;
    for (int i; a < 10; a = a + 1) {
        i = 1;
        a = i + a;
    }
    return a;
}

// 12
int scope_for_2() {
    int a = 0;
    int i = 3;
    for (int i = 0; i < 10; i = i + 1) {
        a = i;
    }
    return a + i;
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

/////////////////////////////////////////////////

char *moji_direct_return() {
    return "文字を返す関数\n";
}

int string_return() {
    printf(moji_direct_return());
    return 8;
}

/////////////////////////////////////////////////

char *moji_global = "moji_global_before\n";

void init_global() {
    moji_global = "moji_global_after\n";
}

// 4
int string_global() {
    printf(moji_global);
    init_global();
    printf(moji_global);
    return 4;
}

// 9
int string_literal_japanese() {
    printf("日本語ですね\n");
    return 9;
}

// 3
int string_literal_ascii_1() {
    char *moji;
    moji = "moji\ndesu\nne\n";
    printf(moji);
    return 3;
}

// 4
int string_literal_ascii_2() {
    char *moji = "haru ha akebono\n";
    char *four = moji + 4;
    char *eight = moji + 8;
    if (four < eight) {
        return eight - four;
    } else {
        return 100;
    }
}

// 2
int string_literal_ascii_3() {
    char *moji = "haru ha akebono\n";
    char *four = moji + 4;
    char *eight = moji + 8;
    if (four + 4 <= eight) {
        return eight - 2 - four;
    } else {
        return 100;
    }
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

// 5
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
    // 最後の文字が使われる
    // 仕様かどうかは不明
    // gccに合わせた
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
    char charara[2][4] = {"abc", "def"};
    char *s1 = charara[0];
    char *s2 = charara[1];
    printf("%s%s\n", s1, s2);
    return 3;
}

// 5
int char_array_and_pointer_6() {
    // 0終端できないとprintfが変になるはず
    char charara[2][3] = {"abc", "def"};
    printf("%s%s\n", charara[0], charara[1]);
    return 5;
}

// 8
int char_array_and_pointer_7() {
//    char charara[][] = {"abc", "def"};
//    char charara[2][] = {"abc", "def"};
    char charara[][4] = {"abc", "def"};
    char *s1 = charara[0];
    char *s2 = charara[1];
    printf("%s%s\n", s1, s2);
    return sizeof(charara);
}

// 33
int char_array_and_pointer_8() {
    char charata[2][3][4] = {
            {
                    {11, 12, 13, 14},
                    {21, 22},
                    {31, 32, 33, 34, 35, 36},
            },
            {
                    {41, 42},
                    {51, 52},
                    {61, 62, 63},
            },
    };
    for (int i = 0; i < 2; i = i + 1) {
        for (int j = 0; j < 3; j = j + 1) {
            for (int k = 0; k < 4; k = k + 1) {
                printf("%d, ", charata[i][j][k]);
                if (k == 3) {
                    printf("\n");
                }
            }
        }
    }
    return 33;
}

// 21
int char_array_and_pointer_9() {
    int a = 0;
    int array[][2] = {{1, 2},
                      {3, 4},
                      {5, 6}};
    for (int i = 0; i < sizeof(array) / sizeof(int); i = i + 1) {
        a = a + array[0][i];
    }
    return a;
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

int *a = 55; // 暗黙のキャストだけどエラーにならないパターン
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

char global_c_1;
char global_c_2;

// 2
int global_variable_5() {
    global_c_1 = 'a';
    global_c_2 = 'c';
    return global_c_2 - global_c_1;
}

int global_int = sizeof(int) + 0;

// 7
int global_variable_6() {
    int b = global_int;
    int global_int = 3;
    int c = global_int;
    return b + c;
}

int *global_p = &global_int + (0 + 3) * 1 - 3;

// 4
int global_variable_7() {
    return *global_p;
}

//int bbb = 3;
//int global_multiple = bbb - 3 + 4 + 1 * 3 * 5; // 19
int global_multiple = 3 - 3 + 4 + 1 * 3 * 5; // 19

int global_division = 35 / 7 + 9; // 14

int global_array_array[2][3];
//int global_array_array[2][3] = {{1, 2, 3}, {4, 5, 6}};
int (*test)[3] = &(global_array_array[1]);
//int failed = global_array_array[1][2];

// 17
int global_variable_8() {
    global_array_array[1][1] = 50;
    int fifty = (*test)[1];
    return fifty - global_multiple - global_division;
}

// 7
int global_variable_9() {
//    int global_array_array[2][3];
    int (*test)[3] = &(global_array_array[1]);
    global_array_array[1][0] = 40;
    int forty = (*test)[0];
    return forty - global_multiple - global_division;
}

/////////////////////////////////////////////////

char global_array_char[4];
char *global_pointer_char_1 = global_array_char + 1;

// 11
int global_variable_10() {
    global_array_char[1] = 11;
    return global_pointer_char_1[0];
}

//int global_array_int_[];
int global_array_int[4];
int *global_array_element_pointer = &global_array_int[2];
//int *global_array_element_pointer = &(global_array_int[2]); // と同じ
//int *global_array_element_pointer = (&global_array_int)[2]; // もビルドは通ることになってる

// 33
int global_variable_11() {
    global_array_int[0] = 0;
    global_array_int[1] = 1;
    global_array_int[2] = 2;
    global_array_int[3] = 33;
    int i = global_array_element_pointer[1];
    printf("%d-------------\n", i);
    return i;
}

int global_compare_1 = 1 < 3;

// 1
int global_variable_12() {
    printf("[compare: 1 < 3] %d-------------\n", global_compare_1);
    return global_compare_1;
}

char global_string[] = "春は曙。やうやう白くなりゆく山際、すこしあかりて、紫だちたる雲の細くたなびきたる。";
// 1
int global_compare_2 = (global_string + 1) < (global_string + 3);
// CLion（clangd）ではerror扱いになっているがgccではビルド通る
int global_compare_3 = 0 < global_string + 3;
//int global_compare_4 = -12 <= global_string + 3; // これはgccでもエラー
int global_compare_4 = 0 <= global_string + 3;
int global_compare_5 = 0 > global_string + 3;
int global_compare_6 = (global_string + 1) >= (global_string + 3);
int global_compare_7 = (&global_string + 5) > (&global_string + 1);

// 1
int global_variable_13() {
    printf("[compare: pointer + 1 < pointer + 3] %d-------------\n", global_compare_2);
    return global_compare_2;
}

// 12
int global_variable_14() {
    if (global_compare_3)
        return 12;
    return 0;
}

// 13
int global_variable_15() {
    if (global_compare_4)
        return 13;
    return 0;
}

// 16
int global_variable_16() {
    if (global_compare_5)
        return 0;
    return 16 + global_compare_6;
}

// 2
int global_offset_1 = (global_string + 3) - (global_string + 1);
// 3
int global_offset_2 = (global_string + 3) - (global_string + 1) + (global_string + 4) - (global_string + 3);
// 4
int global_offset_3 = (&global_string + 5) - (&global_string + 1);

// 2
int global_variable_17() {
    return global_offset_1;
}

// 3
int global_variable_18() {
    return global_offset_2;
}

// 4
int global_variable_19() {
    if (global_compare_7) {
        return global_offset_3;
    }
    return 11;
}

/////////////////////////////////////////////////

int global_array_init_1[4] = {1, 2, 3, 4};
int global_array_init_2[] = {1, 2, 3, 4};
//int global_array_init_x[] = "文字は書けない";
//int global_array_init_x[] = {"文字は書けない", 12, 1};

// 10
int global_variable_20() {
//    char global_string[] = "春は曙。やうやう白くなりゆく山際、すこしあかりて、紫だちたる雲の細くたなびきたる。";
    printf("枕草子: %s\n", global_string);
    return global_array_init_1[0] +
           global_array_init_1[1] +
           global_array_init_1[2] +
           global_array_init_1[3];
}

// 16
int global_variable_21() {
    global_array_init_2[3] = 40;
    // 4 * 4
    return sizeof(global_array_init_2);
}

// 39
int global_variable_22() {
    for (int i = 0; i < sizeof(global_string) / sizeof(char); i = i + 1) {
        int a = i;
        while (26 <= a) {
            a = a - 26;
        }
        global_string[i] = 65 + a;
    }
    printf("枕草子: %s\n", global_string);
    return global_array_init_2[3] - global_array_init_2[0];
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

// 引数なしの宣言と、非ポインタのcharを含む定義および宣言は互換なし
//char chaa();
//char chaa(char c);

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

// 136
int sizeof_9() {
    // 4
    int i = sizeof 1;
    // 4
    int a = sizeof i;
    // 8 (pointer)
    int b = sizeof(int (*)[4][5][6]);
    // 120 = 4 * 5 * 6 * 1 (char)
    int c = sizeof(char[4][5][6]);
    return i + a + b + c;
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

int assert_others();

int assert_others(void) {
    assert("", 99, ({
        int a;
        a = 0;
        int i;
        for (i = 0; i < 10; i = i + 1) a = a + 1;
        99;
    }));
    assert("", 10, ({
        int a;
        a = 0;
        int i;
        for (i = 0; i < 10; i = i + 1) a = a + 1;
        a;
    }));
    assert("", 10, ({
        int a;
        a = 0;
        while (a < 10) a = a + 1;
        a;
    }));

    assert("", 25, ({
        int a_3;
        int _loc;
        a_3 = 12;
        _loc = 3;
        a_3 * _loc - 11;
    }));
    assert("", 5, ({
        int aaa;
        aaa = 3;
        aaa + 2;
    }));
    assert("", 5, ({
        int b29;
        int aaa;
        aaa = 3;
        b29 = 2;
        b29 + aaa;
    }));
    assert("", 5, ({
        int O0;
        O0 = 3;
        O0 = 2;
        O0 + 3;
    }));

    assert("", 5, ({
        int a;
        a = 3;
        a + 2;
    }));
    assert("", 3, ({
        int a;
        a = 3;
        a;
    }));
    assert("", 5, ({
        int a;
        int z;
        a = 3;
        z = 2;
        a + z;
    }));

    assert("", 0, ({ 0; }));
    assert("", 42, ({ 42; }));
    assert("", 42, ({ 40 + 2; }));
    assert("", 0, ({ 100 - 100; }));
    assert("", 10, ({ 100 - 100 + 10; }));
    assert("", 111, ({ 100 + 10 + 1; }));
    assert("", 100, ({ 100 * 10 / 10; }));
    assert("", 101, ({ 100 + 10 / 10; }));
    assert("", 0, ({ 10 * -1 + 10; }));
    assert("", 90, ({ 100 + -1 * 10; }));

    assert("", 0, ({ 0 == 1; }));
    assert("", 1, ({ 42 == 42; }));
    assert("", 1, ({ 0 != 1; }));
    assert("", 0, ({ 42 != 42; }));
    assert("", 1, ({ 42 != 42 + 1; }));
    assert("", 1, ({ 2 + 42 != 42; }));
    assert("", 1, ({ (2 + 42) != 42; }));

    assert("", 1, ({ 0 < 1; }));
    assert("", 0, ({ 1 < 1; }));
    assert("", 0, ({ 2 < 1; }));
    assert("", 1, ({ 0 <= 1; }));
    assert("", 1, ({ 1 <= 1; }));
    assert("", 0, ({ 2 <= 1; }));

    assert("", 1, ({ 1 > 0; }));
    assert("", 0, ({ 1 > 1; }));
    assert("", 0, ({ 1 > 2; }));
    assert("", 1, ({ 1 >= 0; }));
    assert("", 1, ({ 1 >= 1; }));
    assert("", 0, ({ 1 >= 2; }));
}

/////////////////////////////////////////////////

int main() {

    assert("use_struct_1", 13, use_struct_1());
    assert("use_struct_2", 7, use_struct_2());

    assert("scope_for_1", 11, scope_for_1());
    assert("scope_for_2", 12, scope_for_2());
    assert("scoped_1", 1, scoped_1());
    assert("scoped_2", 3, scoped_2());

    // TODO ポインタを返す関数のテストを追加
    assert("string_return", 8, string_return());

    assert("string_global", 4, string_global());
    assert("string_literal_japanese", 9, string_literal_japanese());
    assert("string_literal_ascii_1", 3, string_literal_ascii_1());
    assert("string_literal_ascii_2", 4, string_literal_ascii_2());
    assert("string_literal_ascii_3", 2, string_literal_ascii_3());

    assert("string_literal_char_array_1", 110, string_literal_char_array_1());
    assert("string_literal_char_array_2", 0, string_literal_char_array_2());
    assert("string_literal_char_array_3", 106, string_literal_char_array_3());
    assert("string_literal_char_array_4", 5, string_literal_char_array_4());

    assert("char_literal_1", 5, char_literal_1());
    assert("char_literal_2", 6, char_literal_2());
    assert("char_literal_3", 7, char_literal_3());
    assert("char_literal_4", 9, char_literal_4());

    assert("char_array_and_pointer_1", 13, char_array_and_pointer_1());
    assert("char_array_and_pointer_2", 8, char_array_and_pointer_2());
    assert("char_array_and_pointer_3", 24, char_array_and_pointer_3());
    assert("char_array_and_pointer_4", 4, char_array_and_pointer_4());
    assert("char_array_and_pointer_5", 3, char_array_and_pointer_5());
    assert("char_array_and_pointer_6", 5, char_array_and_pointer_6());
    assert("char_array_and_pointer_7", 8, char_array_and_pointer_7());
    assert("char_array_and_pointer_8", 33, char_array_and_pointer_8());
    assert("char_array_and_pointer_9", 21, char_array_and_pointer_9());
    assert("char_calculate_array", 3, char_calculate_array());

    assert("global_variable_1", 5, global_variable_1());
    assert("global_variable_2", 9, global_variable_2());
    assert("global_variable_3", 3, global_variable_3());
    assert("global_variable_4", 3, global_variable_4());
    assert("global_variable_5", 2, global_variable_5());
    assert("global_variable_6", 7, global_variable_6());
    assert("global_variable_7", 4, global_variable_7());
    assert("global_variable_8", 17, global_variable_8());
    assert("global_variable_9", 7, global_variable_9());

    assert("global_variable_10", 11, global_variable_10());
    assert("global_variable_11", 33, global_variable_11());
    assert("global_variable_12", 1, global_variable_12());
    assert("global_variable_13", 1, global_variable_13());
    assert("global_variable_14", 12, global_variable_14());
    assert("global_variable_15", 13, global_variable_15());
    assert("global_variable_16", 16, global_variable_16());
    assert("global_variable_17", 2, global_variable_17());
    assert("global_variable_18", 3, global_variable_18());
    assert("global_variable_19", 4, global_variable_19());

    assert("global_variable_20", 10, global_variable_20());
    assert("global_variable_21", 16, global_variable_21());
    assert("global_variable_22", 39, global_variable_22());

    assert("array_initialize_1", 0, array_initialize_1());
    assert("array_initialize_2", 2, array_initialize_2());
    assert("array_initialize_3", 4, array_initialize_3());

    assert("array_and_pointer_1", 8, array_and_pointer_1());
    assert("array_and_pointer_2", 8, array_and_pointer_2());
    assert("array_and_pointer_3", 8, array_and_pointer_3());
    assert("array_and_pointer_4", 8, array_and_pointer_4());
    assert("array_and_pointer_5", 12, array_and_pointer_5());
    assert("array_and_pointer_6", 12, array_and_pointer_6());
    assert("array_and_pointer_7", 12, array_and_pointer_7());
    assert("array_and_pointer_8", 12, array_and_pointer_8());
    assert("array_and_pointer_9", 8, array_and_pointer_9());

    assert("array_and_pointer_10", 24, array_and_pointer_10());
    assert("array_and_pointer_11", 3, array_and_pointer_11());
    assert("array_and_pointer_12", 3, array_and_pointer_12());
    assert("array_and_pointer_13", 3, array_and_pointer_13());
    assert("array_and_pointer_14", 3, array_and_pointer_14());
    assert("array_and_pointer_15", 3, array_and_pointer_15());
    assert("array_and_pointer_16", 3, array_and_pointer_16());
    assert("array_and_pointer_17", 3, array_and_pointer_17());
    assert("array_and_pointer_18", 3, array_and_pointer_18());
    assert("array_and_pointer_19", 3, array_and_pointer_19());

    assert("sizeof_1", 4, sizeof_1());
    assert("sizeof_2", 8, sizeof_2());
    assert("sizeof_3", 4, sizeof_3());
    assert("sizeof_4", 8, sizeof_4());
    assert("sizeof_5", 4, sizeof_5());
    assert("sizeof_6", 4, sizeof_6());
    assert("sizeof_7", 4, sizeof_7()); // ★
    assert("sizeof_8", 1, sizeof_8());
    assert("sizeof_9", 136, sizeof_9());

    assert("pointer_and_calculate_1", 3, pointer_and_calculate_1());
    assert("pointer_and_calculate_2", 2, pointer_and_calculate_2());
    assert("pointer_and_calculate_3", 3, pointer_and_calculate_3()); // ★
    assert("pointer_and_calculate_4", 4, pointer_and_calculate_4());
    assert("pointer_and_calculate_5", 3, pointer_and_calculate_5());

    assert("function_1", 13, function_1());
    assert("function_2", 13, function_2());
    assert("function_3", 13, function_3());
    assert("function_4", 13, function_4());
    assert("function_5", 13, function_5());
    assert("function_6", 37, function_6());
    assert("function_7", 12, function_7());
    assert("function_8", 12, function_8());
    assert("function_9", 1, function_9());

    assert("function_10", 13, function_10());
    assert("function_11", 13, function_11());
    assert("function_12", 12, function_12());
    assert("function_13", 11, function_13());
    assert("function_14", 12, function_14());
    assert("function_15", 12, function_15());
    assert("function_16", 12, function_16());
    assert("function_17", 12, function_17());
    assert("function_18", 12, function_18());
    assert("function_19", 12, function_19());

    assert("block_1", 10, block_1());
    assert("block_2", 11, block_2());
    assert("block_3", 11, block_3());
    assert("block_4", 3, block_4());
    assert("block_5", 10, block_5());
    assert("block_6", 10, block_6());
    assert("block_7", 10, block_7());
    assert("block_8", 6, block_8());
    assert("block_9", 6, block_9());

    assert_others();

    printf("=========== ============= ============\n");

    return 0;
}
