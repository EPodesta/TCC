filename=tcc
ratification=ratification/ratificationTCC

all:
	pdflatex ${filename}
	bibtex ${filename}
	pdflatex ${filename}
	pdflatex ${filename}

ratific:
	pdflatex ${ratification}
	bibtex ${ratification}||true
	pdflatex ${ratification}
	pdflatex ${ratification}

open: all
	nohup zathura PFC.pdf &


clean:
	rm -f ${filename}.ps ${filename}.pdf ${filename}.out ${filename}.bbl  \
		  ${filename}.blg ${filename}.acn ${filename}.glo ${filename}.ist \
		  ${filename}.lof ${filename}.log ${filename}.toc ${filename}.idx
	rm -f ${ratification}.ps ${ratification}.pdf ${ratification}.out ${ratification}.bbl  \
		  ${ratification}.blg ${ratification}.acn ${ratification}.glo ${ratification}.ist \
		  ${ratification}.lof ${ratification}.log ${ratification}.toc ${ratification}.idx
	rm -f *.aux *.dvi *.log *.fls *~
