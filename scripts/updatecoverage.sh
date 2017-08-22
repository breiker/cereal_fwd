#!/usr/bin/env bash

# Updates the coverage documentation, and copies it into the appropriate place
# in the gh-pages-develop branch.
#   $1 from CMAKE will contain the root directory of cereal
#   $2 from CMAKE will contain the root directory of build folder

# this requires lcov 1.10 or newer

set -e

COVERAGE_TESTS=./coverage_*

# run tests
for f in $COVERAGE_TESTS
  do
    echo $f
    $f
  done

# build coverage output
tempdir=`mktemp -d`

lcov --capture --directory $2 --output-file coverage.info --no-external --base $1
lcov --remove coverage.info "*/external/*" "*/details/util.hpp" "$1/sandbox/*" "$1/unittests/*" -o coverage.info
genhtml --demangle-cpp coverage.info --output-directory ${tempdir}

echo "output written to ${tempdir}"

pushd $1
# copy over to gh pages
git checkout gh-pages-develop || {
    echo "could not switch branch"
    echo "output written to ${tempdir}"
    popd
    exit 1
    }

popd

rm -rf $1/assets/coverage
mkdir $1/assets/coverage
cp -r ${tempdir}/* $1/assets/coverage/
rm -rf ${tempdir}
