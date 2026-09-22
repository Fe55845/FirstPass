#!/usr/bin/env bash

set -euo pipefail

../build/FirstPass reference/TRAN_Testbench.net tran/results/TRAN_Testbench.csv
./verifier/verifier reference/TRAN_Testbench.csv tran/results/TRAN_Testbench.csv

../build/FirstPass reference/OP_Testbench.net op/results/OP_Testbench.csv
./verifier/verifier reference/OP_Testbench.csv op/results/OP_Testbench.csv

../build/FirstPass reference/AC_Testbench.net ac/results/AC_Testbench.csv
./verifier/verifier reference/AC_Testbench.csv ac/results/AC_Testbench.csv
