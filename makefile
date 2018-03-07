filename=PFC
ratificacao=ratificacaoTCC

all:
	pdflatex ${filename}
	bibtex ${filename}||true
	pdflatex ${filename}
	pdflatex ${filename}

ratific:
	pdflatex ${ratificacao}
	bibtex ${ratificacao}||true
	pdflatex ${ratificacao}
	pdflatex ${ratificacao}

open: all
	zathura PFC.pdf


clean:
	rm -f ${filename}.ps ${filename}.pdf ${filename}.out ${filename}.bbl  \
		  ${filename}.blg ${filename}.acn ${filename}.glo ${filename}.ist \
		  ${filename}.lof ${filename}.log ${filename}.toc ${filename}.idx
	rm -f *.aux *.dvi *.log *.fls *~
