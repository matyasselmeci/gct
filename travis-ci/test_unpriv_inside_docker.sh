#!/bin/bash

IMAGE=$1
COMPONENTS=$2
TARGET=$3

id
env | sort

cd /gct

set +e
args=(--prefix=/gct --enable-silent-rules)
if [[ $COMPONENTS != *ssh* ]]; then
    rm -f prep-gsissh gsi_openssh.gt6.diff
    args+=(--disable-gsi-openssh)
fi
if [[ $COMPONENTS == *gram5* ]]; then
    args+=(--enable-gram5-{server,lsf,sge,slurm,condor,pbs,auditing})
else
    args+=(--disable-gram5)
fi
[[ $COMPONENTS == *myproxy* ]] && args+=(--enable-myproxy)
[[ $COMPONENTS == *udt* ]]     && args+=(--enable-udt)
[[ $IMAGE      == *fedora* ]]  && args+=(LIBS='-ldl')
set -e

echo '================================================================================'
time autoreconf -if
echo '================================================================================'
time ./configure "${args[@]}"
echo '================================================================================'
time make -j $TARGET

echo '================================================================================'
time make -j $TARGET-check | tee check.out
