#!/bin/bash

source "$( dirname -- "$( readlink -f -- "$0"; )"; )/assert.sh"

suite "class should support assignment and acess of data fields"

assertFile "tests/examples/class/data.dojo" 'abc
cba'

suite "class should support assignment and access of methods"

assertFile "tests/examples/class/method.dojo" 'abc
cba
cde'

suite "class should support init and access of initialized data fields"

assertFile "tests/examples/class/init.dojo" 'hello world'

suite "subclass should inherit methods from superclass"

assertFile "tests/examples/class/superclass.dojo" 'grandparent
parent
child'

suite "subclass should shawdow same name methods, including init"

assertFile "tests/examples/class/shadowing.dojo" 'child
still child
nope still child
abc'

suite "local class should work as expected"

assertFile "tests/examples/class/local_class.dojo" 'comment'

suite "should report error if local class is used outside of scope"

assertFileError "tests/examples/class/error_local_class.dojo"

suite "subclass should be able to access parent class's method using super" 

assertFile "tests/examples/class/super.dojo" 'I am the ancestor
parent'

suite "should report error if this or super is used outside of class scope"

assertFileError "tests/examples/class/error_wrong_scope.dojo"