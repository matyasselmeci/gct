#!/bin/bash

id
env | sort

cd /gct
autoreconf -if
./configure --prefix=/gct --enable-myproxy --enable-udt --enable-gram5-{server,lsf,sge,slurm,condor,pbs,auditing}
make
make install

export PATH=/gct/bin:$PATH LD_LIBRARY_PATH=/gct/lib:$LD_LIBRARY_PATH

make check | tee check.out
