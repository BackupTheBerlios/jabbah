#  jabbah = Jabber library for clients
#
#  Copyright (C) 2005 bazyl
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; version 2.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software Foundation,
#  Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  


AM_CFLAGS = -Wall 
AM_LFLAGS = -p -s -i

lib_LTLIBRARIES = libjabbah.la

libjabbah_la_SOURCES = jabbah.h jabbah.c node.h node.c namespace.c \
			presence.h presence.c message.h message.c \
			iq.h iq.c auth.h auth.c sha1.h sha1.c

libjabbah_la_CFLAGS  = $(EXPAT_CFLAGS)
libjabbah_la_LDFLAGS = $(EXPAT_LIBS) 

# libjabbah_a_LIBADD   = pthread.lo expat.lo


