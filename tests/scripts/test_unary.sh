#!/bin/bash

source "$( dirname -- "$( readlink -f -- "$0"; )"; )/assert.sh"

suite "Should negate number"

assert -5 "print(-5)"
assert -42 "print(-42)"
assert -36.9 "print(-36.9)"
assert 36.9 "print(--36.9)"

suite "Should invert boolean"

assert "true" "print(!false)"
assert "false" "print(!true)"
assert "true" "print(!!true)"
assert "true" "print(!!!!true)"
assert "false" "print(!!false)"
assert "false" "print(!!!!false)"

suite "Should invert any value"

assertFile "tests/examples/unary/unary_not.dojo" 'false
false
false
true
true
true
true
true
false
false'