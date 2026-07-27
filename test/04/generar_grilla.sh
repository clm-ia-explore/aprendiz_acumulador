for n in {0..63}; do echo ":: cam-grid[$(((n >> 3 % 8))),$((n % 8))] > 100 :: ../../acc_stim.py cam $n 1 ::" ; done
