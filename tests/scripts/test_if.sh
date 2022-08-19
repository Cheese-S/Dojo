#!/bin/bash

source "$( dirname -- "$( readlink -f -- "$0"; )"; )/assert.sh"

suite "If-else statement should spit out correct result"
assertFile "tests/examples/if/if.dojo" '0
1
2
5
8'

suite "Ternary statement should spit out correct result"
assertFile "tests/examples/if/ternary.dojo" '0
2.5
3'

suite "Should report error if missing parens around condition"
assertFileError "tests/examples/if/error_parens.dojo" 

suite "Should report error if the condition is a statement"
assertFileError "tests/examples/if/error_not_expression.dojo"
