#!/bin/bash
set -e

#export LLVM_INSTALL_PATH=/modules/cs325/llvm-18.1.8
#export LLVM_INSTALL_PATH=/modules/cs325/llvm-20.1.0
export LLVM_INSTALL_PATH=/modules/cs325/llvm-21.1.0
export PATH=$LLVM_INSTALL_PATH/bin:$PATH
export LD_LIBRARY_PATH=$LLVM_INSTALL_PATH/lib:$LD_LIBRARY_PATH
CLANG=$LLVM_INSTALL_PATH/bin/clang++
module load GCC/13.3.0

TEST_COMPILE_ONLY=0

# set compile_only by command line argument -compile_only
# ./tests.sh -compile_only

while [[ $# -gt 0 ]]; do
  case $1 in
    -compile_only)
      TEST_COMPILE_ONLY=1
      shift # past argument
      ;;
    *)
      echo "Unknown option: $1"
      exit 1
      ;;
  esac
done


DIR="$(pwd)"

### Build mccomp compiler
echo "Cleanup *****"
rm -rf ./mccomp

echo "Compile *****"

make clean
make -j mccomp

COMP=$DIR/mccomp
echo $COMP

function validate {
  $1 > perf_out
  echo
  echo $1
  grep "Result" perf_out;grep "PASSED" perf_out
  rc=$?; if [[ $rc != 0 ]]; then echo "TEST FAILED *****";exit $rc; fi;rm perf_out
}

echo "Test *****"

addition=1
factorial=1
fibonacci=1
pi=1
while=1
void=1
cosine=1
unary=1
palindrome=1
recurse=1
rfact=1
null=1
leap=1
# array tests -- set to 1 after completing Part 3 - Grand Finale
array_addition=0
array_func_arg_1d=0
matrix_mul=0
global_array=0


cd tests/addition/

if [ $addition == 1 ]
then	
	cd ../addition/
	pwd
	rm -rf output.ll add
	"$COMP" ./addition.c
    if [ $TEST_COMPILE_ONLY == 0 ]; then
        $CLANG driver.cpp output.ll  -o add
        validate "./add"
    fi
fi


if [ $factorial == 1 ];
then	
	cd ../factorial
	pwd
	rm -rf output.ll fact
	"$COMP" ./factorial.c
    if [ $TEST_COMPILE_ONLY == 0 ]; then
        $CLANG driver.cpp output.ll -o fact
        validate "./fact"
    fi
fi

if [ $fibonacci == 1 ];
then	
	cd ../fibonacci
	pwd
	rm -rf output.ll fib
	"$COMP" ./fibonacci.c
    if [ $TEST_COMPILE_ONLY == 0 ]; then
        $CLANG driver.cpp output.ll -o fib
        validate "./fib"
    fi
fi

if [ $pi == 1 ];
then	
	cd ../pi
	pwd
	rm -rf output.ll pi
	"$COMP" ./pi.c
    if [ $TEST_COMPILE_ONLY == 0 ]; then
        $CLANG driver.cpp output.ll -o pi
        validate "./pi"
    fi
fi

if [ $while == 1 ];
then	
	cd ../while
	pwd
	rm -rf output.ll while
	"$COMP" ./while.c
    if [ $TEST_COMPILE_ONLY == 0 ]; then
        $CLANG driver.cpp output.ll -o while
        validate "./while"
    fi
fi

if [ $void == 1 ];
then	
	cd ../void
	pwd
	rm -rf output.ll void
	"$COMP" ./void.c 
    if [ $TEST_COMPILE_ONLY == 0 ]; then
        $CLANG driver.cpp output.ll -o void
        validate "./void"
    fi
fi

if [ $cosine == 1 ];
then	
	cd ../cosine
	pwd
	rm -rf output.ll cosine
	"$COMP" ./cosine.c
    if [ $TEST_COMPILE_ONLY == 0 ]; then
        $CLANG driver.cpp output.ll -o cosine
        validate "./cosine"
    fi
fi

if [ $unary == 1 ];
then	
	cd ../unary
	pwd
	rm -rf output.ll unary
	"$COMP" ./unary.c
    if [ $TEST_COMPILE_ONLY == 0 ]; then
        $CLANG driver.cpp output.ll -o unary
        validate "./unary"
    fi
fi

if [ $recurse == 1 ];
then	
	cd ../recurse
	pwd
	rm -rf output.ll recurse
	"$COMP" ./recurse.c
    if [ $TEST_COMPILE_ONLY == 0 ]; then
        $CLANG driver.cpp output.ll -o recurse
        validate "./recurse"
    fi
fi

if [ $rfact == 1 ];
then	
	cd ../rfact
	pwd
	rm -rf output.ll rfact
	"$COMP" ./rfact.c
    if [ $TEST_COMPILE_ONLY == 0 ]; then
        $CLANG driver.cpp output.ll -o rfact
        validate "./rfact"
    fi
fi

if [ $palindrome == 1 ];
then	
	cd ../palindrome
	pwd
	rm -rf output.ll palindrome
	"$COMP" ./palindrome.c
    if [ $TEST_COMPILE_ONLY == 0 ]; then
        $CLANG driver.cpp output.ll -o palindrome
        validate "./palindrome"
    fi
fi

if [ $leap == 1 ];
then
        cd ../leap
        pwd
        rm -rf output.ll leap
        "$COMP" ./leap.c
    if [ $TEST_COMPILE_ONLY == 0 ]; then
        $CLANG driver.cpp output.ll -o leap
        validate "./leap"
    fi
fi

if [ $TEST_COMPILE_ONLY == 0 ]; then
    if [ $null == 1 ];
    then	
        cd ../null
        pwd
        rm -rf output.ll null
        "$COMP" ./null.c 
        rc=$?
        if [[ $rc == 0 ]]; then
            echo "null test PASSED"
        else
            echo "null test FAILED"
        fi
    fi
fi

if [ $array_addition == 1 ];
then	
    cd ../array_addition
    pwd
    rm -rf output.ll array_addition
    "$COMP" ./arr_addition.c
    if [ $TEST_COMPILE_ONLY == 0 ]; then
        $CLANG -g driver.cpp output.ll -o array_addition
        validate "./array_addition"
    fi
fi

if [ $array_func_arg_1d == 1 ];
then	
    cd ../array_func_arg_1d
    pwd
    rm -rf output.ll array_func_arg
    "$COMP" ./arr_func_arg.c
    if [ $TEST_COMPILE_ONLY == 0 ]; then
        $CLANG -g driver.cpp output.ll -o array_func_arg
        validate "./array_func_arg"
    fi
fi

if [ $matrix_mul == 1 ];
then	
    cd ../matrix_multiplication
    pwd
    rm -rf output.ll matrix_mul
    "$COMP" ./matrix_mul.c
    if [ $TEST_COMPILE_ONLY == 0 ]; then
        $CLANG -g driver.cpp output.ll -o matrix_mul
        validate "./matrix_mul"
    fi
fi
if [ $global_array == 1 ];
then	
    cd ../global_array
    pwd
    rm -rf output.ll global_array
    "$COMP" ./global_array.c
    if [ $TEST_COMPILE_ONLY == 0 ]; then
        $CLANG -g driver.cpp output.ll -o global_array
        validate "./global_array"
    fi
fi

echo "***** ALL TESTS PASSED *****"
