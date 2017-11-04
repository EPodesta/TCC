set encoding utf8
# Input
set datafile separator " "

# Output
set terminal postscript eps enhanced color size 7.5,2
set output outfile

load "settings.gnuplot"

# X Axis
set xrange[-1:24]
set xtics ("1993" 0, "1994" 1, "1995" 2, "1996" 3, "1997" 4, "1998" 5, "1999" 6, "2000" 7, "2001" 8, "2002" 9, "2003" 10, "2004" 11, "2005" 12, "2006" 13, "2007" 14, "2008" 15, "2009" 16, "2010" 17, "2011" 18, "2012" 19, "2013" 20, "2014" 21, "2015" 22, "2016" 23, "2017" 24)
set xlabel "Ano"

# Y Axis
# set yrange[-1:7]
# set ytic ("100" 0, "1000" 1, "10000" 2, "100000" 3, "1000000" 4, "10000000" 5, "100000000" 6, "1000000000" 7)
set ylabel "Número de núcleos (log)"

set xtics
# set ytics
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
