# Makefile for compiling .tex to pdf

PDF_DIR = pdf

CHAPTERS=$(wildcard chapters/*.tex)

FBM=$(wildcard frontbackmatter/*.tex)

STY=$(wildcard *.sty)

GFX=$(wildcard gfx/*.tex)

all: $(PDF_DIR)/dissertation.pdf

$(PDF_DIR)/%: % $(PDF_DIR)
	@cp -f $< $@	
	
%.pdf : %.tex $(CHAPTERS) $(FBM) $(STY) $(GFX)
	@echo "Generating $*.pdf"
	pdflatex -shell-escape $*
	# biber $*
	#rm -r *.aux chapters/*.aux frontbackmatter/*.aux


$(PDF_DIR):
	@if [ ! -x $(PDF_DIR) ]; \
  then \
    mkdir $(PDF_DIR); \
  fi
  

.PHONY : clean

clean :
	rm -r $(PDF_DIR)/*
	rmdir $(PDF_DIR)
