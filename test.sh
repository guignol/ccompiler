#!/bin/bash

try() {
	assert "$1" "main() { $2 }"
}

assert() {
	expected="$1"
	input="$2"

	cat << END > foo.c
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
END
	gcc -c foo.c

	./9cc "$input" >tmp.s
	gcc -o tmp tmp.s foo.o
	./tmp
	actual="$?"
	
	rm foo.c
	rm foo.o

	if [ "$actual" = "$expected" ]; then
		echo "$input => $actual"
	else
		echo "$input => $expected expected, but got $actual"
		exit 1
	fi
}

make

# assert 13 'main() { return add(1, 12); } add(a, b, a) { hoge(a, b); return a + b; }'
assert 13 'main() { return add(1, 12); } add(a, b) { hoge(a, b); return a + b; }'
assert 13 'main() { return add(1, 12); } add(a, b) { return a + b; }'
# exit 0
assert 13 'main() { return salut(); } salut() { a = 1; b = 12; return 13; }'
assert 13 'main() { return salut(); } salut() { return 13; }'
assert 13 'main() { return bar(13); }'

try 13 'printf(); return 13;'
try 13 'a = 13;  printf(); return a;'
# exit 0

try 12 'b = 1; a = foo() + b;'
try 11 'return foo();'
try 12 'a = foo() + 1;'
try 12 'a = bar(11) + 1;'
try 12 'b = 11; a = bar(b) + 1;'
try 12 'a = hoge(11, 1);'
try 12 'a = 1; b = 11; return hoge(b, a);'
try 12 'hoge(11, 1);'
# exit 0

try 3 '{ aa = 3; { b = 2; } aa; }'

try 10 'a = 0; b = 1; for (i = 0; i < 100; i = i + 1) { a = a + 1; b = a + 1; if (a == 10) return a; } b - 1;'
try 10 'a = 0; b = 1; for (i = 0; i < 10; i = i + 1) { a = a + 1; b = a + 1; } b - 1;'
try 10 'a = 0; b = 1; for (i = 0; i < 10; i = i + 1) { a = a + 1; b = a + 1; } a;'
try 10 'value = 0; for (i = 0; i < 10; i = i + 1) { value = value + 1; }'
# exit 0

try 3 '{ a = 3; }'
try 2 '{ a = 3; b = 2; }'
try 6 '{ a = 3; b = 2; c = 6; }'
try 11 '{ a = 3; b = 2; c = 6; a + b + c; }'
try 11 '{ a = 3; b = 2; c = 6; return a + b + c; }'
try 6 '{ a = 3; b = 2; return c = 6; a + b + c; }'

try 99 'a = 0; for (i = 0; i < 10; i = i + 1) a = a + 1; 99;'
try 10 'a = 0; for (i = 0; i < 10; i = i + 1) a = a + 1;'
try 10 'a = 0; while (a < 10) a = a + 1; return a;'

try 37 'a = 1983; b = 2020; if ((b - a) == 37) return 37; else return 36;'
try 12 'a = 13; if (a == 0) return 3; else return 12;'
try 12 'if (0) 3; else 12;'

try 25 'a_3 = 12; _loc = 3; return a_3 * _loc - 11;'
try 25 'a_3 = 12; _loc = 3; return a_3 * _loc - 11; 24;'

try 5 'aaa = 3; aaa + 2;'
try 5 'aaa = 3; b29 = 2; b29 + aaa;'
try 5 'O0 = 3; O0 = 2; O0 + 3;'

try 5 'a = 3; a + 2;'
try 3 'a = 3; a;'
try 3 '3;'
try 5 'a = 3; z = 2; a + z;'

# try 0 '1 + aaa'
try 0 "0;"
try "42" "42;"
try 42 "40+2;"
try 0 '100-100;'
try 10 '100-100+10;'
try 111 '100 + 10 +1;'
# try 4 "+"
try 100 '100 * 10 / 10;'
try 101 '100 + 10 / 10;'
# try 111 '100 + 10 / 10'
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
