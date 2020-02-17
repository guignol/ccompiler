#!/bin/bash

COUNT=$#

try() {
  assert "$1" "int main() { $2 }"
}

assert() {
  expected="$1"
  input="$2"

  ./build/ccompiler "$input" >tmp.s
  gcc -o tmp tmp.s build/libfoo.a -no-pie
  if [ "$COUNT" == 0 ]; then
    ./tmp >/dev/null
  else
    ./tmp
  fi
  actual="$?"

  printf "▓"

  if [ "$actual" != "$expected" ]; then
    echo
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

# TODO メモ
# popとpushは64bitのレジスタやアドレスのみ
# mov rax, [rbp-4]はできるが
# mov [rbp-8], raxは足りない分がゼロクリアされる
# mov DWORD PTR [rbp-4], 5で即値を32bit枠に入れられる
# TODO アセンブリのビルドと実行と確認
# $ gcc -o tmp tmp.s -no-pie; ./tmp; echo $?
# TODO gdb入門
# $ gdb ./tmp
# (gdb) display /4i $pc
# (gdb) start
# (gdb) stepi

#make
# For CMake 3.13 or later, use these options to set the source and build folders
# $ cmake -B/path/to/my/build/folder -S/path/to/my/source/folder
# For older CMake, use some undocumented options to set the source and build folders:
# $ cmake -B/path/to/my/build/folder -H/path/to/my/source/folder
# https://stackoverflow.com/a/24435795
/usr/bin/cmake -DCMAKE_BUILD_TYPE=Debug -G "CodeBlocks - Unix Makefiles" -Bbuild/ -H.
/usr/bin/cmake --build ./build --target ccompiler -- -j 4
/usr/bin/cmake --build ./build --target foo -- -j 4

#assert 8 "$(
#  cat <<END
#int main() {
#}
#END
#)"

assert 9 "$(
  cat <<END
int main() {
  printf("日本語ですね\n");
  return 9;
}
END
)"

assert 3 "$(
  cat <<END
int main() {
  char *moji;
  moji = "moji\ndesu\nne\n";
  printf(moji);
  return 3;
}
END
)"

assert 13 "$(
  cat <<END
int main() {
  char a[4];
  char (*b)[4];
  b = &a;
  (*b)[3] = 13;
  return (*b)[3];
}
END
)"

assert 8 "$(
  cat <<END
int main() {
  char a[2][3];
  char (*b)[3];
  b = &a;
  a[1][2] = 5;
  return a[1][2] + 3;
}
END
)"

assert 1 "$(
  cat <<END
char chaa(char c) {
  return c + 1;
}
int main() {
  char c;
  c = sizeof(c);
  int i;
  i = chaa(c);
  return i - c;
}
END
)"

assert 3 "$(
  cat <<END
int main() {
  char x[3];
  x[0] = -1;
  x[1] = 2;
  int y;
  y = 4;
  return x[0] + y;
}
END
)"

#assert 8 "$(
#  cat <<END
#int main() {
#  return a();
#}
#END
#)"

#assert 7 "$(
#  cat <<END
#int deja;
#int deja() {
#  return 3;
#}
#int main() {
#  return deja() + 4;
#}
#END
#)"

assert 5 "$(
  cat <<END
int *a;
int *b;
int main() {
  int a; // 型の異なるローカル変数
  a = 3;
  b =  &a;
  *b = 2 + a;
  return a;
}
END
)"

assert 9 "$(
  cat <<END
int a;
int *b;
int main() {
  a = 1;
  b =  &a;
  *b = 8 + a;
  return a;
}
END
)"

assert 3 "$(
  cat <<END
int a;
int init() {
  a = 6;
}
int main() {
  int b;
  init();
  b = a;
  int a;
  a = 3;
  return b - a;
}
END
)"

assert 3 "$(
  cat <<END
int a;
int main() {
  a = 3;
  int b;
  b = 6;
  return b - a;
}
END
)"

assert 8 "$(
  cat <<END
int main() {
  int a[2][3];
  int *b;
  int (*c)[4];
  b = a;
  c = a;
  a[1][2] = 5;
  return a[1][2] + 3;
}
END
)"

assert 8 "$(
  cat <<END
int main() {
  int a[2][3];
  int (*b)[3];
  b = &a;
  a[1][2] = 5;
  return a[1][2] + 3;
}
END
)"

assert 8 "$(
  cat <<END
int main() {
  int a[2][3];
  int (*b)[3];
  b = a;
  a[1][2] = 5;
  return a[1][2] + 3;
}
END
)"

assert 8 "$(
  cat <<END
int main() {
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
END
)"

assert 12 "$(
  cat <<END
int main() {
  int a[2][3];
  int *b;
  int c;
  c = 1;
  b = &c;
  (*b)[a][2] = 12;
  return((*b)[a])[2];
}
END
)"

assert 12 "$(
  cat <<END
int main() {
  int a[2][3];
  int b[5];
  b[4] = 1;
  (a[0])[b[4]] = 12;
  return (a[0])[1];
}
END
)"

assert 12 "$(
  cat <<END
int main() {
  int a[2][3][4];
  int one;
  one = 1;
  a[0][one][2] = 7;
  one[a][2][3] = 5;
  return a[0][one][2] + a[one][2][3];
}
END
)"

assert 12 "$(
  cat <<END
int main() {
  int a[2][3];
  a[0][0] = 11;
  a[0][1] = 12;
  a[0][2] = 13;
  a[1][0] = 21;
  a[1][1] = 22;
  a[1][2] = 23;
  return a[0][1];
}
END
)"

assert 8 "$(
  cat <<END
int main() {
  int a[2];
  int b;
  b = sizeof(a);
  return b;
}
END
)"

assert 24 "$(
  cat <<END
int main() {
  int a[2][3];
  int b;
  b = sizeof(a);
  return b;
}
END
)"

assert 3 "$(
  cat <<END
int main() {
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
END
)"

assert 3 "$(
  cat <<END
int main() {
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
END
)"

assert 3 "$(
  cat <<END
int main() {
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
END
)"

assert 3 "$(
  cat <<END
int main() {
  int a[2];
  0[a] = 1;
  1[a] = 2;
  int *p;
  p = a;
  int maji;
  maji = 0[p]; // ポインタにも[]使える
  return maji + 1[a];  // 3
}
END
)"

assert 3 "$(
  cat <<END
int main() {
  int a[5];
  a[0] = 1;
  a[1] = 2;
  a[2] = 3;
  a[3] = 4;
  a[4] = 5;
  int maji;
  maji = a[1 + 3]; // ポインタにも[]使える
  return maji - a[1];  // 3
}
END
)"

assert 3 "$(
  cat <<END
int main() {
  int a[2];
  a[0] = 1;
  a[1] = 2;
  int *p;
  p = a;
  int maji;
  maji = p[0]; // ポインタにも[]使える
  return maji + a[1];  // 3
}
END
)"

assert 3 "$(
  cat <<END
int main() {
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
END
)"

assert 3 "$(
  cat <<END
int main() {
  int a[2];
  *a = 3;
  return *a;
}
END
)"

assert 3 "$(
  cat <<END
int main() {
  int a[2];
  *a = 1;
  *(a + 1) = 2;
  int *p;
  p = a;
  return *p + *(p + 1);  // 3
}
END
)"

#assert 3 "$(
#  cat <<END
#int main() {
#  int *a;
#  *a = 1; // 未定義動作っぽい？
#  return 1;
#}
#END
#)"

try 4 'int x; return sizeof(x);'
try 8 'int *y; return sizeof(y);'
try 4 'int x; return sizeof(x + 3);'
try 8 'int *y; return sizeof(y + 3);'
try 4 'int *y; return sizeof(*y);'
# sizeofに渡す式は何でもよい
try 4 'return sizeof(1);'
# sizeofの結果は現在int型なのでsizeof(int)と同じ
try 4 'return sizeof(sizeof(1));'

assert 3 "$(
  cat <<END
int main() {
  int *p;
  int *q;
  alloc_array_4(&p, 0, 1, 2, 3);
  // foo(*p);
  q = p + 3;
  return q - p;
}
END
)"

assert 2 "$(
  cat <<END
int main() {
  int *p;
  alloc_array_4(&p, 0, 1, 2, 3);
  foo(*p);
  p = p + 1;
  p = 1 + p;
  return *p;
}
END
)"

assert 3 "$(
  cat <<END
int main() {
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
END
)"

assert 4 "$(
  cat <<END
int main() {
  int x;
  int *y;
  x = 3;
  y = &x;
  *y = 4;
  return x;
}
END
)"

assert 3 "$(
  cat <<END
int main() {
	int x;
	x = 3;
	return *(&x);
}
END
)"

try 10 'int a; a = 0; int b; b = 1; int i; for (i = 0; i < 100; i = i + 1) { a = a + 1; b = a + 1; if (a == 10) return a; } return b - 1;'
try 11 'int a; a = 3; int b; b = 2; int c; c = 6; return a + b + c; '
try 11 'int a; int b; int c; { a = 3; b = 2; c = 6; return a + b + c; }'

assert 1 "$(
  cat <<END
int fibonacci(int n) {
	if (n == 0)	{
		return 1;
	} else if (n == 1) {
		return 1;
	}
	return fibonacci(n - 1) + fibonacci(n - 2);
}

int main() {
  int i;
	for (i = 0; i < 10; i = i + 1) 	{
		hoge(i, fibonacci(i));
	}
	return 1;
}
END
)"
# exit 0

# assert 13 'int main() { return add(1, 12); } add(int a, int b, int a) { hoge(a, b); return a + b; }'
assert 13 'int add(int a, int b) { hoge(a, b); return a + b; }int main() { return add(1, 12); }'
assert 13 'int add(int a, int b) { return a + b; } int main() { return add(1, 12); }'
assert 13 'int salut() { int a; int b; a = 1; b = 12; return 13; } int main() { return salut(); } '
assert 13 'int salut() { return 13; } int main() { return salut(); } '
assert 13 'int main() { return bar(13); }'

try 13 'printf("moji: %i", 13); return 13;'
try 13 'int a; a = 13;  printf("moji: %i", 13); return a;'
# exit 0

try 12 'int b; b = 1; int a; a = foo() + b; return a;'
try 11 'return foo();'
try 12 'int a; return a = foo() + 1;'
try 12 'int a; return a = bar(11) + 1;'
try 12 'int a; int b; b = 11; return a = bar(b) + 1;'
try 12 'int a; return a = hoge(11, 1);'
try 12 'int a; int b; a = 1; b = 11; return hoge(b, a);'
try 12 'return hoge(11, 1);'
# exit 0

try 3 '{ int aa; int b; aa = 3; { b = 2; } return aa; }'

try 10 'int a; int b; int i; a = 0; b = 1; for (i = 0; i < 10; i = i + 1) { a = a + 1; b = a + 1; } return b - 1;'
try 10 'int a; int b; int i; a = 0; b = 1; for (i = 0; i < 10; i = i + 1) { a = a + 1; b = a + 1; } return a;'
try 10 'int value; int i; value = 0; for (i = 0; i < 10; i = i + 1) { value = value + 1; } return value;'
# exit 0

try 3 '{ int a; return a = 3; }'
try 2 '{ int a; int b; a = 3; return b = 2; }'
try 6 '{ int a; int b; int c; a = 3; b = 2; return c = 6; }'
try 6 '{ int a; int b; int c; a = 3; b = 2; return c = 6; return a + b + c; }'

try 99 'int a; a = 0; int i; for (i = 0; i < 10; i = i + 1) a = a + 1; return 99;'
try 10 'int a; a = 0; int i; for (i = 0; i < 10; i = i + 1) a = a + 1; return a;'
try 10 'int a; a = 0; while (a < 10) a = a + 1; return a;'

try 37 'int a; int b; a = 1983; b = 2020; if ((b - a) == 37) return 37; else return 36;'
try 12 'int a; a = 13; if (a == 0) return 3; else return 12;'
try 12 'if (0) return 3; else return 12;'

try 25 'int a_3; int _loc; a_3 = 12; _loc = 3; return a_3 * _loc - 11;'
try 25 'int a_3; int _loc; a_3 = 12; _loc = 3; return a_3 * _loc - 11; 24;'

try 5 'int aaa; aaa = 3; aaa + 2;'
try 5 'int b29; int aaa; aaa = 3; b29 = 2; b29 + aaa;'
try 5 'int O0; O0 = 3; O0 = 2; O0 + 3;'

try 5 'int a; a = 3; a + 2;'
try 3 'int a; a = 3; a;'
try 3 'return 3;'
try 5 'int a; int z; a = 3; z = 2; return a + z;'

# try 0 '1 + aaa'
try 0 "return 0;"
try "42" "return 42;"
try 42 "40+2;"
try 0 '100-100;'
try 10 '100-100+10;'
try 111 '100 + 10 +1;'
try 100 '100 * 10 / 10;'
try 101 '100 + 10 / 10;'
try 0 '10 * -1 + 10;'
try 90 '100 + -1 * 10;'

try 0 '0 == 1;'
try 1 '42 == 42;'
try 1 '0 != 1;'
try 0 '42 != 42;'
try 1 '42 != 42 + 1;'
try 1 '2 + 42 != 42;'
try 1 '(2 + 42) != 42;'

try 1 '0 < 1;'
try 0 '1 < 1;'
try 0 '2 < 1;'
try 1 '0 <= 1;'
try 1 '1 <= 1;'
try 0 '2 <= 1;'

try 1 '1 > 0;'
try 0 '1 > 1;'
try 0 '1 > 2;'
try 1 '1 >= 0;'
try 1 '1 >= 1;'
try 0 '1 >= 2;'

echo OK
