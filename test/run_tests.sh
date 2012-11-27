#!/bin/sh
#
#  Compile and run unit tests
#
#  ./run_tests.sh [-v]               -- compiles and runs all tests
#  ./run_tests.sh [-v] SOME_FILE.CPP -- compiles and runs only one test
#
#  -v  -- Run tests under valgrind
#  

set -e

CXX="g++"
CXXFLAGS="-g -Wall -Wextra -Wredundant-decls -Wdisabled-optimization -pedantic -Wctor-dtor-privacy -Wnon-virtual-dtor -Woverloaded-virtual -Wsign-promo"
COMPILE="$CXX -I../include -I. $CXXFLAGS -o tests"

if [ "x$1" = "x-v" ]; then
    VALGRIND="valgrind --leak-check=full --show-reachable=yes"
    shift
else
    VALGRIND=""
fi

BOLD="[1m"
NORM="[0m"
GREEN="[1;32m"
DARKRED="[31m"
RED="[1;31m"

TESTS_COMPILE_ERROR=0
TESTS_FAILED=0
TESTS_OK=0

CFLAGS="$(geos-config --cflags) $(gdal-config --cflags)"
LIBS="$(geos-config --libs) $(gdal-config --libs) -Lboost_lib -lboost_regex -lboost_iostreams -lboost_context"

test_file () {
    FILES="test_main.o test_utils.o $1"
    #eval CFLAGS=`../get_options.sh --cflags $FILES`
    #eval LIBS=`../get_options.sh --libs $FILES`
    echo -n "Checking $BOLD$1$NORM..."
    if ! output=$($COMPILE $FILES $CFLAGS $LIBS -lboost_unit_test_framework 2>&1 )
    then
        echo "$DARKRED[COMPILE ERROR]$NORM"
        TESTS_COMPILE_ERROR=$(($TESTS_COMPILE_ERROR+1))
        echo "=========================="
        echo $COMPILE $FILES $CFLAGS $LIBS -lboost_unit_test_framework
        echo "--------------------------"
        echo "$output"
        echo "=========================="
        return
    fi

    if ! output=$($VALGRIND ./tests 2>&1 )
    then
        echo "$RED[TEST FAILED]$NORM"
        TESTS_FAILED=$(($TESTS_FAILED+1))
        echo "=========================="
        echo "$output"
        echo "=========================="
        return
    fi
    echo "$GREEN[SUCCESS]$NORM"
    TESTS_OK=$((TESTS_OK+1))
}

setup() {
    if [ \( ! -e test_main.o \) -o \( test_main.cpp -nt test_main.o \) ]
    then
        echo "Compiling test runner"
        $CXX -I../include -I. $CXXFLAGS -c test_main.cpp 
    fi
    if [ \( ! -e test_utils.o \) -o \( test_utils.cpp -nt test_utils.o \) ]
    then
        echo "Compiling test helper"
        $CXX -I../include -I. $CXXFLAGS -c test_utils.cpp
    fi 
}

my_path=`dirname $0`
cd $my_path
setup
if [ "x$1" = "x" ]; then
    for FILE in t/*/test_*.cpp; do
        test_file $FILE
    done
else
    test_file $1
fi

if [ $(($TESTS_COMPILE_ERROR + $TESTS_FAILED)) = 0 ]
then
    echo "all tests succeeded"
    exit 0
else
    echo "some tests failed"
    echo "$TESTS_OK ok, $TESTS_COMPILE_ERROR compile error, $TESTS_FAILED fail"
    exit 1
fi
