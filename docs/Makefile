all :
	rm -vf docs.{aux,bbl,blg,dvi,ilg,ind,lof,log,lot,ps,pdf,toc,nlo,nls}
	pdflatex docs.tex
	#makeindex docs
	pdflatex docs.tex
	makeindex docs
	pdflatex docs.tex

#pdf :
#	pdflatex docs.tex

v :
	evince docs.pdf || atril docs.pdf || open docs.pdf

view : v

latex2html :
	latex2html -show_section_numbers -local_icons -split 2 docs.tex

