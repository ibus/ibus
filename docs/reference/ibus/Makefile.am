# vim:set noet ts=4:
#
# ibus - The Input Bus
#
# Copyright (c) 2007-2015 Peng Huang <shawn.p.huang@gmail.com>
# Copyright (c) 2007-2020 Red Hat, Inc.
# Copyright (c) 2015-2020 Takao Fujiwara <takao.fujiwara1@gmail.com>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
# USA

# We require automake 1.6 at least.
AUTOMAKE_OPTIONS = 1.6

# This is a blank Makefile.am for using gtk-doc.
# Copy this to your project's API docs directory and modify the variables to
# suit your project. See the GTK+ Makefiles in gtk+/docs/reference for examples
# of using the various options.

# The name of the module, e.g. 'glib'.
DOC_MODULE=ibus

# The top-level SGML file. You can change this if you want to.
DOC_MAIN_SGML_FILE=$(DOC_MODULE)-docs.sgml

# The directory containing the source code. Relative to $(srcdir).
# gtk-doc will search all .c & .h files beneath here for inline comments
# documenting the functions and macros.
# e.g. DOC_SOURCE_DIR=$(top_srcdir)/gtk
DOC_SOURCE_DIR=$(top_srcdir)/src $(top_builddir)/src

# Extra options to pass to gtkdoc-scangobj. Not normally needed.
SCANGOBJ_OPTIONS=

# Extra options to supply to gtkdoc-scan.
# e.g. SCAN_OPTIONS=--deprecated-guards="GTK_DISABLE_DEPRECATED"
SCAN_OPTIONS= --rebuild-sections

# Extra options to supply to gtkdoc-mkdb.
# e.g. MKDB_OPTIONS=--xml-mode --output-format=xml
MKDB_OPTIONS=--xml-mode --output-format=xml

# Extra options to supply to gtkdoc-mktmpl
# e.g. MKTMPL_OPTIONS=--only-section-tmpl
MKTMPL_OPTIONS=

# Extra options to supply to gtkdoc-fixref. Not normally needed.
# e.g. FIXXREF_OPTIONS=--extra-dir=../gdk-pixbuf/html --extra-dir=../gdk/html
FIXXREF_OPTIONS=

# Used for dependencies. The docs will be rebuilt if any of these change.
# e.g. HFILE_GLOB=$(top_srcdir)/gtk/*.h
# e.g. CFILE_GLOB=$(top_srcdir)/gtk/*.c
HFILE_GLOB=$(top_srcdir)/src/*.h
CFILE_GLOB=$(top_srcdir)/src/*.c

# Header files to ignore when scanning. Use base file/dir names
# e.g. IGNORE_HFILES=gtkdebug.h gtkintl.h
IGNORE_HFILES= \
	ibuscomposetable.h \
	ibusconfigprivate.h \
	ibusinternal.h \
	keyname-table.h \
	keysymdef.h \
	ibusenumtypes.h \
	stamp-ibusenumtypes.h \
	ibusmarshalers.h \
	stamp-ibusmarshalers.h \
	$(NULL)

# Images to copy into HTML directory.
# e.g. HTML_IMAGES=$(top_srcdir)/gtk/stock-icons/stock_about_24.png
HTML_IMAGES=

# Extra SGML files that are included by $(DOC_MAIN_SGML_FILE).
# e.g. content_files=running.sgml building.sgml changes-2.0.sgml
content_files=

# SGML files where gtk-doc abbrevations (#GtkWidget) are expanded
# These files must be listed here *and* in content_files
# e.g. expand_content_files=running.sgml
expand_content_files=

# CFLAGS and LDFLAGS for compiling gtkdoc-scangobj with your library.
# Only needed if you are using gtkdoc-scangobj to dynamically query widget
# signals and properties.
# e.g. GTKDOC_CFLAGS=-I$(top_srcdir) -I$(top_builddir) $(GTK_DEBUG_FLAGS)
# e.g. GTKDOC_LIBS=$(top_builddir)/gtk/$(gtktargetlib)
GTKDOC_CFLAGS= \
	@GLIB2_CFLAGS@ \
	@GOBJECT2_CFLAGS@ \
	@GIO2_CFLAGS@ \
	@DBUS_CFLAGS@ \
	-I$(top_srcdir)/src \
	-I$(top_builddir)/src \
	$(NULL)
GTKDOC_LIBS= \
	@GLIB2_LIBS@ \
	@GOBJECT2_LIBS@ \
	@GIO2_LIBS@ \
	@DBUS_LIBS@ \
	$(top_builddir)/src/libibus-@IBUS_API_VERSION@.la \
	$(NULL)

# This includes the standard gtk-doc make rules, copied by gtkdocize.
include $(top_srcdir)/gtk-doc.make

# Other files to distribute
# e.g. EXTRA_DIST += version.xml.in
EXTRA_DIST += trim.sed

# Files not to distribute
# for --rebuild-types in $(SCAN_OPTIONS), e.g. $(DOC_MODULE).types
# for --rebuild-sections in $(SCAN_OPTIONS) e.g. $(DOC_MODULE)-sections.txt
DISTCLEANFILES = $(DOC_MODULE)-sections.txt

# Comment this out if you want 'make check' to test you doc status
# and run some sanity checks
if ENABLE_GTK_DOC
TESTS_ENVIRONMENT = cd $(srcdir) && \
  DOC_MODULE=$(DOC_MODULE) DOC_MAIN_SGML_FILE=$(DOC_MAIN_SGML_FILE) \
  SRCDIR=$(abs_srcdir) BUILDDIR=$(abs_builddir)
#TESTS = $(GTKDOC_CHECK)
endif

trim-build.stamp: scan-build.stamp
	$(AM_V_GEN) \
	    $(SED) -f $(srcdir)/trim.sed -i.bak \
			$(builddir)/$(DOC_MODULE)-sections.txt && \
	    $(RM) $(builddir)/$(DOC_MODULE)-sections.txt.bak && \
	    touch trim-build.stamp

tmpl-build.stamp: trim-build.stamp  $(DOC_MODULE)-decl.txt $(SCANOBJ_FILES) $(DOC_MODULE)-overrides.txt

# clean-local:
# 	rm -rf tmpl ${DOC_MODULE)

CLEANFILES+= *actions *.stamp

-include $(top_srcdir)/git.mk
