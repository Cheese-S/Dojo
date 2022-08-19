#!/bin/bash

source "$( dirname -- "$( readlink -f -- "$0"; )"; )/assert.sh"

suite "Should work with integers"

assert 7 "print 10 - 3"
assert 5 "print 10 / 2"
assert 7 "print 5 + 2" 
assert 1 "print 1 * 1"

suite "Should work with floats"

assert 15 "print 15.5 - 0.5"
assert 20 "print 15.5 + 4.5"
assert 6.4 "print 12.8 / 2.0"
assert 10.33 "print 103.3 * 0.1"

suite "Should work with multiple same precedence operators"

assert 15 "print 10 + 10 - 2 -3"
assert 20 "print 20 / 5 / 4 * 20"

suite "Should work with mixed precedence operators"

assert 15 "print 30 - 30 / 2"
assert 20 "print 0 + 25 / 5 + 15"
assert 35 "print 25 / 5 + 30"

suite "Should work with parenthese"

assert 15 "print 3 * (2 + 3)"
assert 15 "print (0 + 25) / 5 + 10"
assert 0 "print (((0)))"

suite "Comparison should work"

assert "false" "print 3 > 5"
assert "false" "print 3 >= 5"
assert "false" "print 5.5 < 3.5"
assert "false" "print 5.5 <= 3.5"
assert "true" "print 5.5 >= 3.5"
assert "true" "print 5.5 >= 5.5"
assert "true" "print 3.5 <= 5.5"
assert "true" "print 3.5 <= 3.5"
assert "true" "print 3.5 == 3.5"
assert "true" "print 3.5 != 3.6"
assert "false" "print 3.5 == 3.6"
assert "false" "print 3.5 != 3.5"

suite "'&&' and '||' should give the correct result"
assertFile "tests/examples/binary/logical_binary.dojo" '0
0
nil
5'
