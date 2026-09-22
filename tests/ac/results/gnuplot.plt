set terminal pngcairo size 1920,1080 enhanced font 'Verdana,30'

set output 'AC_Testbench.png'
set datafile separator ","
set title "High Order RLC Network"
set dummy jw, y
set grid nopolar
set grid layerdefault   lt 0 linecolor 0 linewidth 2,  lt 0 linecolor 0 linewidth 2
set key inside bottom center

set xlabel "f" 
set ylabel "magnitude of V(N100)" 
set y2label "Phase of V(N100) (degrees)" 
set y2tics border out scale 1,0.5 nomirror norotate  autojustify

set logscale x 10
set colorbox vertical origin screen 0.9, 0.2 size screen 0.05, 0.6 front  noinvert bdefault

plot "AC_Testbench.txt" using 1:2 with lines lw 4 lc "orange" title "LTSpice ABS", "AC_Testbench.txt" using 1:3 with lines axes x1y2 lw 4 lc "green" title "LTSpice ARG" ,\
	"AC_Testbench.csv" using 1:(log10($200)*20) with lines lw 2 lc "red" title "FirstPass ABS", "AC_Testbench.csv" using 1:($201*180/pi) with lines axes x1y2 lw 2 lc "dark-green" title "FirstPass ARG"
