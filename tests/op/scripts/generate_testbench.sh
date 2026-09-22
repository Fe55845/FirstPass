#!/usr/bin/env bash

set -euo pipefail

g++ generate_op_testbench.cpp -O3
./a.out