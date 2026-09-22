# ------------------------------------------------------
#   Signalverlauf Forward Converter – Auswertung
#   Datei: SIM1.txt
# ------------------------------------------------------

set terminal pngcairo size 1920,1080 enhanced font 'Verdana,30'     

set output 'TRAN_Testbench.png'

set datafile separator ","

set grid lt 1 lw 10 lc "gray"
set key top right box opaque
set xlabel 'Zeit [s]'
set xrange [0:*]

set title "Tran Testbench"

set ylabel 'Voltage N18 / V'
plot 'TRAN_Testbench.csv' using ($1):($20) with lines lw 4 lc 'blue' title "FirstPass" ,\
     'TRAN_Testbench.txt' using ($1):($2) with lines lw 2 lc 'light-blue' title "LTspice"
