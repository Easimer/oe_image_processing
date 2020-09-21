TEX=pdflatex
BIB=biber
BASE=specification
CIT=cite.bib

all: $(BASE).pdf

%.tex: %.raw
	./raw2tex $< > $@

%.tex: %.dat
	./dat2tex $< > $@

$(BASE).pdf: $(BASE).tex
	latexmk -pdf -pdflatex="pdflatex -interaction=nonstopmode" -use-make $(BASE).tex

run: $(BASE).pdf
	xdg-open $(BASE).pdf

clean:
	latexmk -CA

.PHONY: $(BASE).pdf all run clean
