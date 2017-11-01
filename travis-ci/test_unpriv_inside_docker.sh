#!/bin/bash

id

cd /gct
autoreconf -if
./configure
make

env | sort

make -j1 check | tee check.out
