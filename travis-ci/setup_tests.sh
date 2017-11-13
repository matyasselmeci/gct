#!/bin/sh -xe

# This script starts docker

sudo docker run --rm=true -v `pwd`:/gct:rw centos:centos${OS_VERSION} /bin/bash -c "bash -xe /gct/travis-ci/test_inside_docker.sh ${OS_VERSION} ${COMPONENTS}"

