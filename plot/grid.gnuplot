set encoding utf8
# Input
set datafile separator " "

# Output
set terminal postscript eps enhanced color size 7.5,2
set output outfile

load "settings.gnuplot"

# set offset 0,0,graph 0.5, graph 0.5
# X Axis
set xrange[-1:15]
set xtics ("1993" 0, "1996" 1, "1997" 2, "1999" 3, "2000" 4, "2002" 5, "2004" 6, "2005" 7, "2007" 8, "2008" 9, "2009" 10, "2010" 11, "2011" 12, "2012" 13, "2013" 14, "2016" 15)
set xlabel "Ano"

# Y Axis
# set yrange[-1:6]
# set ytics (100, 500, 1000, 5000, 10000, 30000, 50000, 80000, 100000, 500000, 1000000)
# set for [i=0:7] ytics (0,10**i)
# set ylabel "Número de núcleos (log)"


set xtics
set ytics
# Grid
set grid xtics
set grid ytics

set macros

# set ylabel label

unset key

YAXIS = "set format y yformat; \
		 set yrange [0:]"

LEGEND = "set key inside top right height 1 width 1 box lw 1 font ',12' samplen 1.5"

@YAXIS
@LEGEND
    plot data using ($2/scale)  with linespoints title "Fur" ls 16
