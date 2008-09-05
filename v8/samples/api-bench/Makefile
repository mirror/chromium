INCDIR = ../../src/api

ifneq ($(MAKECMDGOALS),clean)
ifndef LIBDIR
$(error Invoke using 'make LIBDIR=<path to libv8.a>')
endif
endif

all:	transitions benchpress

transitions: transitions.cc $(LIBDIR)/libv8.a
	$(CXX) -O2 -fomit-frame-pointer -g -o $@ -I$(INCDIR) $< \
		-L$(LIBDIR) -lv8 -lpthread

benchpress: api-benchpress.cc $(LIBDIR)/libv8.a
	$(CXX) -O2 -fomit-frame-pointer -g -o $@ -I$(INCDIR) $< \
		-L$(LIBDIR) -lv8 -lpthread

clean:
	$(RM) -f transitions benchpress
