
all:
	@(cd sipgen; $(MAKE))
	@(cd siplib; $(MAKE))

install:
	@(cd sipgen; $(MAKE) install)
	@(cd siplib; $(MAKE) install)
	@test -d $(DESTDIR)/usr/lib/python2.7/site-packages || mkdir -p $(DESTDIR)/usr/lib/python2.7/site-packages
	cp -f sipconfig.py $(DESTDIR)/usr/lib/python2.7/site-packages/sipconfig.py
	cp -f /usr/src/sip/sipdistutils.py $(DESTDIR)/usr/lib/python2.7/site-packages/sipdistutils.py

clean:
	@(cd sipgen; $(MAKE) clean)
	@(cd siplib; $(MAKE) clean)
