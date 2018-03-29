ngp
===

**n**curses **g**re**p**

ngp lets you look for a pattern and display results in ncurses.

ngp lets you browse results with ease.

ngp lets you open a result with your favorite editor at the right line.

Build and Install
------------

1. Install build dependencies for your platform/distribution : `libconfig`, `libpcre` & `ncurses`

2. Enter the following commands in your terminal :

```
git clone git@github.com:jonathanklee/ngp.git
cd ./ngp
cmake .
make
make install
```

3. That's it. You can now open a terminal and enjoy !

```
ngp my_pattern
```

Native or External parser
----------
ngp implements it's own parser but you can also use it with an external parser.

By default, ngp looks into the extensions listed in the ngprc configuration file.
If you want to override those extensions with a specific extension, you can use the -t option :
```
ngp my_pattern -t my_extension
```
For instance, we can look for the *LOCAL_MODULE* expression in Android Open Source Project makefiles.
```
ngp LOCAL_MODULE -t mk
```
To look into all the files, the native parser has also a raw option.
```
ngp my_pattern -r
```
To look for a regular expression, the native parser implements the -e option.
```
ngp my_regexp -e
```
For instance, we can look for a regular expression like *^*._MODULE :=.*$*
```
ngp "^.*_MODULE :=.*$" -e
```
Have a look at ```ngp -h``` for more options.

You can also use an external tool to parse your source code.
Currently only `ag` and `git grep` are supported.

The default parser can be specified in your ~/.config/ngp/ngprc file.
Other available values for the 'default_parser' option are "ag" or "git". The parser can also be specified using command line arguments.
For example, `ngp --ag -- pattern` will use `ag` as a parser anbd will overwrite the 'default_parser' setting.
Please check `ngp --help` for further information on command line arguments.

Your ngprc file also allows you to customize the commands for `ag` and `git grep`. Thus, you can add options for `ag` like "-C"
or you can change a tools location if it's not in your $PATH.
Note that it is mandatory to specify the three arguments : options, pattern and path for each command.

Demo
-------

Looking for "create" pattern in ngp source code.

[![asciicast](https://asciinema.org/a/2r4kmqt572knj5m271z6o7b0y.png)](https://asciinema.org/a/2r4kmqt572knj5m271z6o7b0y)

License
----
```
Copyright (c) 2013 Jonathan Klee

ngp is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

ngp is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with ngp.  If not, see <http://www.gnu.org/licenses/>.
```

