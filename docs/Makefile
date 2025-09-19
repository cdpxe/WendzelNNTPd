# List of the mardown files which should be included in the pdf documentation.
# The order of the files corresponds to the order of the chapters in the PDF.
DOCFILES=docs/index.md docs/install.md docs/configuration.md docs/running.md docs/development.md docs/upgrade.md

all : site docs.pdf

site : mkdocs.yml docs/*
	mkdocs build

docs.pdf : docs/*
	pandoc --from markdown+gfm_auto_identifiers+footnotes $(DOCFILES) --template template.tex --top-level-division=chapter --lua-filter=convert-links.lua -s -o docs.pdf

v : docs.pdf
	evince docs.pdf || atril docs.pdf || open docs.pdf

view : v

serve-html :
	mkdocs serve

clean :
	rm -f docs.pdf
	rm -rf site/
