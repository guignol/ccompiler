#!/bin/bash
try() {
  expected="$1"
  input="$2"

  ./9cc "$input" > tmp.s
  gcc -o tmp tmp.s
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

make

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