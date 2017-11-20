SOURCEPATH=$1
PLOTDIR=$2
label=$3
scale=$4
type=$5
yformat="%1.1f"

            gnuplot                                                            \
                -e "label='$label'"                               \
                -e "yformat='$yformat'"                                        \
                -e "scale='$scale'"                                            \
                -e "data='./tmpgreen.txt'"                             \
                -e "outfile='$PLOTDIR/PreviewPlot.eps'"                        \
            grid.gnuplot

epstool --copy --bbox $PLOTDIR/PreviewPlot.eps --output $PLOTDIR/${type}500.eps
epstopdf                     \
    --outfile=$PLOTDIR/${type}500.pdf  \
    $PLOTDIR/${type}500.eps

rm -r $PLOTDIR/PreviewPlot.eps
rm -r $PLOTDIR/${type}500.eps
rm -r ./tmpTime.txt
