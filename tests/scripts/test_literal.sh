#!/bin/bash

source "$( dirname -- "$( readlink -f -- "$0"; )"; )/assert.sh"

suite "Number Literal"

assert 5 "print 5"
assert 42 "print 42"
assert 36.9 "print 36.9"

suite "Boolean Literal"

assert "false" "print false"
assert "true" "print true"
assert "nil" "print nil"


suite "String Literal"

assert "string" 'print '\"'string'\"
assert "" 'print '\"\"
assert "" 'print '\`\`
assert "true" 'print '\`'string'\`'=='\`'string'\`
assert "true" 'print '\"'string'\"'=='\"'string'\"

suite "Template String"

assert "head false mid true end" 'print '\`'head ${false} mid ${true} end'\`
assert "head 8 mid 1 end" 'print '\`'head ${'\`'${5 + 3}'\`'} mid ${'\`'${3 - 2}'\`'} end'\`