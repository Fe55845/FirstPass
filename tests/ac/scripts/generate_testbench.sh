#!/usr/bin/env bash

set -euo pipefail

g++ generate_ac_testbench.cpp -O3
./a.out