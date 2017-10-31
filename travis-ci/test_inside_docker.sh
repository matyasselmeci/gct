#!/bin/bash

OS_VERSION=$1

set -xe

# Clean the yum cache
yum clean all

# First, install all the needed packages.
rpm -Uvh https://dl.fedoraproject.org/pub/epel/epel-release-latest-${OS_VERSION}.noarch.rpm

# Broken mirror?
echo "exclude=mirror.beyondhosting.net" >> /etc/yum/pluginconf.d/fastestmirror.conf

# TODO: don't download openssh and we can get rid of curl
yum -y install gcc gcc-c++ curl make autoconf automake libtool \
               libtool-ltdl-devel openssl-devel git patch \
               'perl(Test::More)' 'perl(File::Spec)' 'perl(URI)' \
               openssl
# ^ openssl-devel doesn't bring in openssl on el7

getent passwd builduser > /dev/null || useradd builduser
chown -R builduser: /gct
su builduser -c '/bin/bash -xe /gct/travis-ci/test_unpriv_inside_docker.sh'

set +x
echo "**** TESTS COMPLETED *****"
cd /gct
grep '^FAIL:\|^PASS:\|^SKIP:' check.out
if grep -q '^FAIL:' check.out; then
    echo "**** `grep -c '^FAIL:' check.out` TESTS FAILED ****"
    find . -wholename \*/test/test-suite.log | while read line; do
        echo "=== $line ==="
        cat "$line"
        echo
    done
    exit 1
fi
