#!/usr/bin/env bash

set -euo pipefail

g++ generate_tran_testbench.cpp -O3
./a.out
