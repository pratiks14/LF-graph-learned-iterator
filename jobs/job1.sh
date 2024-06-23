#!/bin/sh

python3 ../benchmark/bm_script_threads.py > op_threads.txt
python3 ../benchmark/bm_script_nodes.py > op_nodes.txt
python3 ../benchmark/bm_script_snapshot_per.py > op_snapshot_per.txt