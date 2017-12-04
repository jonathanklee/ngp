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

The Parser
----------
ngp implements it's own native source code parser.
However you can use an external tool for parsing your source code.
Currently `ag` and `git grep` are supported.

The default parser can be specified in the .ngprc file.
It's preset is "nat", the native parser.
Other available values for the 'default_parser' option are "ag" or "git".
The parser can also be specified using command line arguments.
For example `ngp --ag -- pattern` will use `ag` for the search, which will overwrite the 'default_parser' setting.
Please check `ngp --help` for further information on command line arguments.

The .ngprc file also allows customizing the commands for `ag` and `git grep`.
Thereby you can set default parser options, like "-C".
Or you can change a tools location if it's not in the $PATH.
However it is mandatory to specify the three arguments, options, pattern and path for each command.

Example
-------

Looking for "create" pattern in ngp source code.

[![asciicast](https://asciinema.org/a/2r4kmqt572knj5m271z6o7b0y.png)](https://asciinema.org/a/2r4kmqt572knj5m271z6o7b0y)
