#!/bin/bash

source "$( dirname -- "$( readlink -f -- "$0"; )"; )/assert.sh"

suite "Should negate number"

assert -5 -5
assert -42 -42
assert -36.9 -36.9
assert 36.9 --36.9 

suite "Should invert boolean"

assert "true" "!false"
assert "false" "!true"
assert "true" "!!true"
assert "true" "!!!!true"
assert "false" "!!false"
assert "false" "!!!!false"
