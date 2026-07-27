#!/usr/bin/env bash

../acc_init.py demo 16 --force
../acc_stim.py demo 0 20
../acc_stim.py demo 5 8
../acc_stim.py demo 10 -4
../acc_stim.py demo 15 40
../acc_view.py demo --method minmax --format int
../acc_view.py demo --method sigmoid --scale 10 --format int
../acc_view.py demo --method softmax --temperature 0.5 --format float
../acc_view.py demo --method posmax --format heat
../acc_view.py demo --method rank --format csv
../acc_view.py demo --method minmax --format heat --no-spaces

../acc_mem.py memorize demo demo.dat --force
../acc_mem.py reconstruct demo demo.dat --force
../acc_mem.py reconstruct demo_copy demo.dat --force
../acc_view.py demo_copy --method minmax --format int

../acc_quant.py demo.dat demo.qdat --method minmax --force
../acc_quant.py demo.dat demo.qdat --method sigmoid --scale 10 --min 0 --max 255 --force
../acc_quant.py demo.dat demo.qdat --method softmax --temperature 0.3 --force
../acc_quant.py demo.dat demo.qdat --method softmax --renorm-max --force

od -An -td8 -j24 demo.dat
od -An -td4 -j32 demo.qdat
