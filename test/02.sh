#!/usr/bin/env bash

../acc_init.py test 25 --force

../acc_stim.py test 12 30
../acc_stim.py test 7 12
../acc_stim.py test 18 -8
../acc_stim.py test 0 5

../acc_view.py test --method minmax --format int
../acc_view.py test --method sigmoid --scale 10 --format int
../acc_view.py test --method softmax --temperature 0.4 --format float
../acc_view.py test --method posmax --format heat --no-spaces

../acc_mem.py memorize test test.dat --force

../acc_quant.py test.dat test.qdat --method minmax --force
../acc_quant.py test.dat test_softmax.qdat --method softmax --temperature 0.4 --renorm-max --force
