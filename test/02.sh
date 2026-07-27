#!/usr/bin/env bash

cd "$(dirname "$0")"

../python_impl/acc_init.py test 25 --force

../python_impl/acc_stim.py test 12 30
../python_impl/acc_stim.py test 7 12
../python_impl/acc_stim.py test 18 -8
../python_impl/acc_stim.py test 0 5

../python_impl/acc_view.py test --method minmax --format int
../python_impl/acc_view.py test --method sigmoid --scale 10 --format int
../python_impl/acc_view.py test --method softmax --temperature 0.4 --format float
../python_impl/acc_view.py test --method posmax --format heat --no-spaces

../python_impl/acc_mem.py memorize test test.dat --force

../python_impl/acc_quant.py test.dat test.qdat --method minmax --force
../python_impl/acc_quant.py test.dat test_softmax.qdat --method softmax --temperature 0.4 --renorm-max --force
