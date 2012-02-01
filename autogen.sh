#! /bin/sh

	autoheader && autoconf
	(cd libmd ; autoheader && autoconf)
