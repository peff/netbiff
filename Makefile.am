SUBDIRS = netbiff backend

EXTRA_DIST = \
	doc/config.txt \
	doc/gui.txt \
	doc/netbiffd \
	doc/netbiffd-imap \
	doc/protocol.txt \
	images/flagup.xpm \
	images/flagdown.xpm \
	images/yes.xpm \
	images/no.xpm \
	netbiff.spec.in \
	debian/changelog \
	debian/control \
	debian/copyright \
	debian/dirs \
	debian/docs \
	debian/rules

dist-hook: netbiff.spec
	cp netbiff.spec $(distdir)

