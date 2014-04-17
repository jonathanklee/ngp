#! /bin/sh

aclocal \
&& touch AUTHORS NEWS ChangeLog \
&& automake --add-missing \
&& autoconf
