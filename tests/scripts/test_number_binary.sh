#!/bin/bash

source "$( dirname -- "$( readlink -f -- "$0"; )"; )/assert.sh"

suite "Should work with integers"

assert 7 "10 - 3"
assert 5 "10 / 2"
assert 7 "5 + 2" 
assert 1 "1 * 1"

suite "Should work with floats"

assert 15 "15.5 - 0.5"
assert 20 "15.5 + 4.5"
assert 6.4 "12.8 / 2.0"
assert 10.33 "103.3 * 0.1"

suite "Should work with multiple same precedence operators"

assert 15 "10 + 10 - 2 -3"
assert 20 "20 / 5 / 4 * 20"

suite "Should work with mixed precedence operators"

assert 15 "30 - 30 / 2"
assert 20 "0 + 25 / 5 + 15"
assert 35 "25 / 5 + 30"

suite "Should work with parenthese"

assert 15 "3 * (2 + 3)"
assert 15 "(0 + 25) / 5 + 10"
assert 0 "(((0)))"