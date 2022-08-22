#!/bin/bash

source "$( dirname -- "$( readlink -f -- "$0"; )"; )/assert.sh"

suite "funtion decalaration and call expression should work as expected"

assertFile "tests/examples/functions/fn.dojo" '11
This is the name of the function <fn function>
21'

suite "Closure should get/set their corresponding upvalue"

assertFile "tests/examples/functions/closure.dojo" 'one
two'

suite "Call a function with the wrong number of arguments should cause an error"

assertFileError "tests/examples/functions/error_wrong_arity.dojo"

suite "Stack should overflow when nested function calls"

assertFileError "tests/examples/functions/error_stack_overflow.dojo"



