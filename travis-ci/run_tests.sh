#!/bin/bash -xe

if [[ $COMPILE_RESULT -ne 0 ]]; then
    echo "***** COMPILE FAILED, NOT RUNNING TESTS *****"
    exit $COMPILE_RESULT
fi

catfile () {
    echo "===== $1 ====="
    cat "$1"
    echo
}

make globus_gram_job_manager-check  |  tee check.out
echo "**** TESTS COMPLETED *****"
grep '^FAIL:\|^PASS:\|^SKIP:\|^ERROR:' check.out
count=`grep -c '^FAIL:\|^ERROR:' check.out` || :
if [[ $count -ge 1 ]]; then
    echo "**** $count TESTS FAILED ****"
    find . -wholename \*/test/test-suite.log | while read logfile; do
        catfile "$logfile"
    done
    echo "========== JOBMANAGER TEST LOGS =========="
    find gram/jobmanager -wholename \*/test/\*.log | while read logfile; do
        catfile "$logfile"
    done
    exit 1
fi

