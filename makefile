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
	rm -f ${filename}.{ps,pdf,out,bbl,blg,acn,glo,ist,}
	rm -f *.aux *.dvi *.log *.fls *~
