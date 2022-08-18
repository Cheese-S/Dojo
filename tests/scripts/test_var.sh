#!/bin/bash

source "$( dirname -- "$( readlink -f -- "$0"; )"; )/assert.sh"

suite "Should define and get global"

assertFile "tests/examples/var/global.dojo" '6
7
This is a template string 6'

suite "Should define and get local"

assertFile "tests/examples/var/local.dojo" '6
5'

suite "Should assign to local and global correctly"

assertFile "tests/examples/var/assignment.dojo" '8
6'

suite "Should stop local refer to itself in initialization"

assertFileError "tests/examples/var/error_local.dojo" 

suite "Should stop global refer to itself in initalization"

assertFileError "tests/examples/var/error_global.dojo" 

suite "Should stop invalid assignment"

assertFileError "tests/examples/var/error_assignment.dojo" 
