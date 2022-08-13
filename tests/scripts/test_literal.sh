#!/bin/bash

source "$( dirname -- "$( readlink -f -- "$0"; )"; )/assert.sh"

suite "Number Literal"

assert 5 5
assert 42 42
assert 36.9 36.9

suite "Boolean Literal"

assert "false" "false"
assert "true" "true"
assert "nil" "nil"


suite "String Literal"

assert "string" \"'string'\"
assert "" \"\"
assert "" \`\`

suite "Template String"