## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = xbuffy
xbuffy_SOURCES = xbuffy.c boxfile.c nntp.c imap.c realfrom.c remfirstwd.c header_cmp.c fmt_rfc2047.c patchlevel.h  xbuffy.h
EXTRA_DIST = XBuffy.ad boxfile.fmt boxfile.sample xbuffy.xbm xbuffy.1 README.imap README.cclient
man_MANS = xbuffy.1

#SUBDIRS = libdyn
xbuffy_LDADD = @LIBOBJS@  @X_PRE_LIBS@ @X_LIBS@ @X_EXTRA_LIBS@ -lXt -lXmu -lXaw -lXext -lX11 -lHX

DEFS= @DEFS@

