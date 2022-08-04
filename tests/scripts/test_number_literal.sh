#!/bin/bash
DOJO="./build/dojo"
assert() {
  expected="$1"
  input="$2"

  echo $expected > test.dojo

  actual=`$DOJO test.dojo`

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}


assert 5 5
assert 42 42
assert 36.9 36.9

rm -f test.dojo

echo OK