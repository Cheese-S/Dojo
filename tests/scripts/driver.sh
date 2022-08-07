#!/bin/bash

source "$( dirname -- "$( readlink -f -- "$0"; )"; )/assert.sh"

tests=`find ./tests/scripts -name 'test_*.sh'`

checks=0
failures=0

for script in $(find ./tests/scripts -name 'test_*.sh')
do
    ((checks++))
    chmod u+x $script
    name="[TEST] $(basename $script .sh)"
    echo ${name^^}
    $script
    if [[ $? -gt 0 ]]
    then 
        ((failures++))
        printRedText "FAILED"
    else
        printGreenText "OK"
    fi
    echo '-----------------'
    cleanup
done

echo "Checks: $checks, Failures: $failures"


