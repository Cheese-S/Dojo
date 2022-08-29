#!/bin/bash

source "$( dirname -- "$( readlink -f -- "$0"; )"; )/assert.sh"

suite "literals"

assertFile "tests/examples/literals/literal.dojo" 'false
true
nil
5
5.5
Of course it ignores whitespace as well!
multiline
string'

suite "string template should interpolate expressions and strings properly"

assertFile "tests/examples/literals/string_template.dojo" 'This is a template string false it works! true see for yourself 4.25 dojo'