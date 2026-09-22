#!/usr/bin/env bash

set -euo pipefail

g++ -O3 -std=c++17 verifier.cpp -o verifier
./verifier ../OP_Testbench.csv ../../OP_Testfiles/results/OP_Testbench.csv