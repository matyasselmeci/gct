#!/bin/bash

id
env | sort

cd /gct
autoreconf -if
./configure --enable-myproxy
make

make check | tee check.out
