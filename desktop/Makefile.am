# Top-level automake makefile. Hands the real work to src/Makefile.am.

ACLOCAL_AMFLAGS = -I m4
SUBDIRS = src

dist_doc_DATA = README.md

# A couple of niceties for developers.
.PHONY: run cleanall

run: all
	cd src && ./tagstack-desktop

cleanall: distclean
	rm -rf autom4te.cache build-aux Makefile.in src/Makefile.in aclocal.m4 \
	       configure config.log config.status
