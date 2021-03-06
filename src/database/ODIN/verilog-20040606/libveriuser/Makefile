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
#ident "$Id: Makefile.in,v 1.33 2004/02/10 19:25:00 steve Exp $"
#
#
SHELL = /bin/sh


prefix = /usr/local
exec_prefix = ${prefix}
srcdir = .



bindir = ${exec_prefix}/bin
libdir = ${exec_prefix}/lib
libdir64 = ${exec_prefix}/lib
includedir = $(prefix)/include

CC = gcc
INSTALL = /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644

CPPFLAGS = -DHAVE_CVS_IDENT=1 -I. -I.. -I$(srcdir) -I$(srcdir)/..  -DHAVE_CONFIG_H -fPIC
CFLAGS = -Wall -g -O2
LDFLAGS = 
STRIP = @STRIP@
RANLIB = ranlib

A = a_close.o a_compare_handles.o a_configure.o a_fetch_argc.o \
a_fetch_argv.o a_fetch_dir.o a_fetch_fullname.o a_fetch_location.o \
a_fetch_param.o a_fetch_range.o a_fetch_tfarg.o a_fetch_time.o \
a_fetch_type.o a_fetch_type_str.o a_fetch_value.o a_handle_by_name.o \
a_handle_hiconn.o a_handle_object.o a_handle_parent.o \
a_handle_simulated_net.o a_handle_tfarg.o a_initialize.o \
a_next.o a_next_bit.o a_next_port.o a_next_topmod.o \
a_object_of_type.o a_product_version.o \
a_set_value.o a_vcl.o a_version.o

O = asynch.o delay.o exprinfo.o finish.o getcstringp.o getinstance.o \
getlongp.o getp.o getsimtime.o io_print.o math.o mc_scan_plusargs.o \
nodeinfo.o nump.o putlongp.o putp.o spname.o typep.o workarea.o \
veriusertfs.o priv.o $A

ifeq (,yes)
ALL32 = all32
INSTALL32 = install32
UNINSTALL32 = uninstall32
endif

all: dep libveriuser.a $(ALL32)

check: all

Makefile: Makefile.in config.status
	./config.status

libveriuser.o: $O
	$(LD) -r -o $@ $O

libveriuser.a: libveriuser.o
	rm -f $@
	ar cvq $@ libveriuser.o
	$(RANLIB) $@

dep:
	mkdir dep

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -MD -c $< -o $*.o
	mv $*.d dep


clean:
	rm -rf *.o dep libveriuser.a libveriuser.o

distclean: clean
	rm -f Makefile config.status config.log config.cache

install:: all installdirs $(libdir64)/libveriuser.a $(INSTALL32)

$(libdir64)/libveriuser.a: ./libveriuser.a
	$(INSTALL_DATA) ./libveriuser.a $(libdir64)/libveriuser.a

installdirs: mkinstalldirs
	$(srcdir)/mkinstalldirs $(includedir) $(libdir64)

uninstall::
	rm -f $(libdir64)/libveriuser.a

ifeq (,yes)
all32: bin32 bin32/libveriuser.a

bin32:
	mkdir bin32

bin32/%.o: %.c
	$(CC) -m32 $(CPPFLAGS) $(CFLAGS) -MD -c $< -o $@

bin32/libveriuser.o: $(addprefix bin32/,$O)
	$(LD) -melf_i386 -r -o $@ $(addprefix bin32/,$O)

bin32/libveriuser.a: bin32/libveriuser.o
	rm -f $@
	ar cvq $@ bin32/libveriuser.o
	$(RANLIB) $@

install32: $(libdir)/libveriuser.a

$(libdir)/libveriuser.a: bin32/libveriuser.a
	$(INSTALL_DATA) bin32/libveriuser.a $(libdir)/libveriuser.a

-include $(patsubst bin32/%.o, bin32/%.d, $(addprefix bin32/,$O))
endif

-include $(patsubst %.o, dep/%.d, $O)
