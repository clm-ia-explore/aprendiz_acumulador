#!/usr/bin/env bash

cd "$(dirname "$0")"

../python_impl/acc_init.py demo 16 --force
../python_impl/acc_stim.py demo 0 20
../python_impl/acc_stim.py demo 5 8
../python_impl/acc_stim.py demo 10 -4
../python_impl/acc_stim.py demo 15 40
../python_impl/acc_view.py demo --method minmax --format int
../python_impl/acc_view.py demo --method sigmoid --scale 10 --format int
../python_impl/acc_view.py demo --method softmax --temperature 0.5 --format float
../python_impl/acc_view.py demo --method posmax --format heat
../python_impl/acc_view.py demo --method rank --format csv
../python_impl/acc_view.py demo --method minmax --format heat --no-spaces

../python_impl/acc_mem.py memorize demo demo.dat --force
../python_impl/acc_mem.py reconstruct demo demo.dat --force
../python_impl/acc_mem.py reconstruct demo_copy demo.dat --force
../python_impl/acc_view.py demo_copy --method minmax --format int

../python_impl/acc_quant.py demo.dat demo.qdat --method minmax --force
../python_impl/acc_quant.py demo.dat demo.qdat --method sigmoid --scale 10 --min 0 --max 255 --force
../python_impl/acc_quant.py demo.dat demo.qdat --method softmax --temperature 0.3 --force
../python_impl/acc_quant.py demo.dat demo.qdat --method softmax --renorm-max --force

od -An -td8 -j24 demo.dat
od -An -td4 -j32 demo.qdat
