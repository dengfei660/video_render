#!/bin/bash

BASE=$(pwd)

#get all commit count
COMMIT_COUNT=$(git rev-list HEAD --count)
echo $COMMIT_COUNT

#get current commit id
COMMIT_ID=$(git show -s --pretty=format:%h)
echo $COMMIT_ID

#find the commit id line
COMMIT_COUNT_LINE=`sed -n '/#define COMMITCNT/=' aml_version.h`
echo $COMMIT_COUNT_LINE

#find the commit id line
COMMIT_ID_LINE=`sed -n '/#define COMMITID/=' aml_version.h`
echo $COMMIT_ID_LINE

#instead the commit count
sed -i -e ${COMMIT_COUNT_LINE}s"/.*/#define COMMITCNT ${COMMIT_COUNT}/" aml_version.h

#instead the original commit id
sed -i -e ${COMMIT_ID_LINE}s"/.*/#define COMMITID ${COMMIT_ID}/" aml_version.h

