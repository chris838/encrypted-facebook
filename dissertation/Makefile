# Makefile for compiling .tex to pdf

PDF_DIR = pdf

SRCS=$(wildcard *.tex)

CHAPTERS=$(wildcard Chapters/*.tex)

OBJS=$(patsubst %.tex, $(PDF_DIR)/%.pdf, $(SRCS))

all: $(OBJS)

$(PDF_DIR)/%: %
	@cp -f $< $@
	
%.pdf : %.tex customstyles.sty $(CHAPTERS) $(PDF_DIR)
	@echo "Generating $*.pdf"
	pdflatex $*.tex $(PDF_DIR)
	@rm -r $*.log $*.aux
	
%.sty :
	@echo "needed sty"

$(PDF_DIR):
	@if [ ! -x $(PDF_DIR) ]; \
  then \
    mkdir $(PDF_DIR); \
  fi
  

.PHONY : clean

clean :
	rm -r $(PDF_DIR)/*
	rmdir $(PDF_DIR)
