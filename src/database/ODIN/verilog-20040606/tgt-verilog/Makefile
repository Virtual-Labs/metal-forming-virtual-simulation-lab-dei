#
#    This source code is free software; you can redistribute it
#    and/or modify it in source code form under the terms of the GNU
#    Library General Public License as published by the Free Software
#    Foundation; either version 2 of the License, or (at your option)
#    any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU Library General Public License for more details.
#
#    You should have received a copy of the GNU Library General Public
#    License along with this program; if not, write to the Free
#    Software Foundation, Inc.,
#    59 Temple Place - Suite 330
#    Boston, MA 02111-1307, USA
#
#ident "$Id: Makefile.in,v 1.12 2004/02/10 19:25:01 steve Exp $"
#
#
SHELL = /bin/sh

VERSION = 0.0

prefix = /usr/local
exec_prefix = ${prefix}
srcdir = .



bindir = ${exec_prefix}/bin
libdir = ${exec_prefix}/lib
includedir = $(prefix)/include

CC = gcc
INSTALL = /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644

CPPFLAGS = -DHAVE_CVS_IDENT=1 -I.. -I$(srcdir)/..  -DHAVE_CONFIG_H -fPIC
CFLAGS = -Wall -g -O2
LDFLAGS = 

all: dep verilog.tgt

dep:
	mkdir dep

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -MD -c $< -o $*.o
	mv $*.d dep

O = verilog.o

ifeq (no,yes)
  TGTLDFLAGS= -L.. -livl
  TGTDEPLIBS=../libivl.a
else
  TGTLDFLAGS=
  TGTDEPLIBS=
endif

verilog.tgt: $O $(TGTDEPLIBS)
	$(CC) -shared -o $@ $O $(TGTLDFLAGS)

clean:
	rm -rf *.o dep verilog.tgt

distclean: clean
	rm -f Makefile

install: all installdirs $(libdir)/ivl/verilog.tgt \
	$(includedir)/vpi_user.h

$(libdir)/ivl/verilog.tgt: ./verilog.tgt
	$(INSTALL_PROGRAM) ./verilog.tgt $(libdir)/ivl/verilog.tgt


installdirs: ../mkinstalldirs
	$(srcdir)/../mkinstalldirs $(libdir)/ivl

uninstall:
	rm -f $(libdir)/ivl/verilog.tgt


-include $(patsubst %.o, dep/%.d, $O)
