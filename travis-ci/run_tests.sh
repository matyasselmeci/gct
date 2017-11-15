#!/bin/bash -xe

if [[ $COMPILE_RESULT -ne 0 ]]; then
    echo "***** COMPILE FAILED, NOT RUNNING TESTS *****"
    exit $COMPILE_RESULT
fi

make -j check  |  tee check.out
echo "**** TESTS COMPLETED *****"
grep '^FAIL:\|^PASS:\|^SKIP:' check.out
count=`grep -c '^FAIL:' check.out` || :
if [[ $count -ge 1 ]]; then
    echo "**** $count TESTS FAILED ****"
    find . -wholename \*/test/test-suite.log | while read logfile; do
        echo "=== $logfile ==="
        cat "$logfile"
        echo
    done
    exit 1
fi

