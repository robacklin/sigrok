#!/bin/sh
##
## This file is part of the sigrok-firmware-fx2lafw project.
##
## Copyright (C) 2013 Uwe Hermann <uwe@hermann-uwe.de>
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
##

OS=`uname`

if [ "x$OS" = "xDarwin" ]; then
	if [ -d /sw/share/aclocal ]; then
		# fink installs aclocal macros here
		ACLOCAL_DIR="-I /sw/share/aclocal"
	elif [ -d /opt/local/share/aclocal ]; then
		# Macports installs aclocal macros here
		ACLOCAL_DIR="-I /opt/local/share/aclocal"
	elif [ -d /usr/local/share/aclocal ]; then
		# Homebrew installs aclocal macros here
		ACLOCAL_DIR="-I /usr/local/share/aclocal"
	elif [ -d /usr/share/aclocal ]; then
		# Xcode installs aclocal macros here
		ACLOCAL_DIR="-I /usr/share/aclocal"
	fi
fi

echo "Generating build system..."
mkdir -p autostuff
aclocal ${ACLOCAL_DIR} || exit 1
autoheader || exit 1
automake --add-missing --copy || exit 1
autoconf || exit 1

