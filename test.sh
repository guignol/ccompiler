#!/bin/bash

try() {
	assert "$1" "int main() { $2 }"
}

assert() {
	expected="$1"
	input="$2"

	./build/ccompiler "$input" >tmp.s
	gcc -o tmp tmp.s build/libfoo.a
	./tmp
	actual="$?"

	if [ "$actual" = "$expected" ]; then
		echo "$input => $actual"
	else
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
# $ gcc -o tmp tmp.s; ./tmp; echo $?
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
int main() {
  int i;
	for (i = 0; i < 10; i = i + 1) 	{
		hoge(i, fibonacci(i));
	}
	return 1;
}

int fibonacci(int n) {
	if (n == 0)	{
		return 1;
	} else if (n == 1) {
		return 1;
	}
	return fibonacci(n - 1) + fibonacci(n - 2);
}
END
)"
# exit 0

# assert 13 'int main() { return add(1, 12); } add(int a, int b, int a) { hoge(a, b); return a + b; }'
assert 13 'int main() { return add(1, 12); } int add(int a, int b) { hoge(a, b); return a + b; }'
assert 13 'int main() { return add(1, 12); } int add(int a, int b) { return a + b; }'
assert 13 'int main() { return salut(); } int salut() { int a; int b; a = 1; b = 12; return 13; }'
assert 13 'int main() { return salut(); } int salut() { return 13; }'
assert 13 'int main() { return bar(13); }'

try 13 'printf(); return 13;'
try 13 'int a; a = 13;  printf(); return a;'
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
