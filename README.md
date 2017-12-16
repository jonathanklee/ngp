ngp
===

Ncurses grep tool.

ngp lets you look for a pattern in your source code directory and display results in ncurses.

ngp lets you browse results with ease.

ngp lets you open a result with your favorite editor at the right line.

Installation
------------

1. Install build dependencies for your platform/distribution : `libconfig`, `libpcre` & `ncurses`

2. Enter the following commands in your terminal :

```
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
ngp implements it's own code source parser. You can also use an external tool to parse your source code.
Currently only `ag` and `git grep` are supported.

The default parser can be specified in your ~/.ngprc file.
Other available values for the 'default_parser' option are "ag" or "git". The parser can also be specified using command line arguments.
For example, `ngp --ag -- pattern` will use `ag` as a parser anbd will overwrite the 'default_parser' setting.
Please check `ngp --help` for further information on command line arguments.

Your ~/.ngprc file also allows you to customize the commands for `ag` and `git grep`. Thus, you can add options for `ag` like "-C"
or you can change a tools location if it's not in your $PATH.
Note that it is mandatory to specify the three arguments : options, pattern and path for each command.

Example
-------

Looking for "create" pattern in ngp source code.

[![asciicast](https://asciinema.org/a/2r4kmqt572knj5m271z6o7b0y.png)](https://asciinema.org/a/2r4kmqt572knj5m271z6o7b0y)
