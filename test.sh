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
try 0 0
try "42" "42"
try 42 "40+2"
try 0 '100-100'
try 10 '100-100+10'
try 111 '100 + 10 +1'
# try 4 "+"
try 100 '100 * 10 / 10'
try 101 '100 + 10 / 10'
# try 111 '100 + 10 / 10'

echo OK