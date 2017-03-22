ngp
===

Ncurses grep tool.

ngp lets you look for a pattern in your source code directory and display results in ncurses.

ngp lets you browse results in a Vim-like style.

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

3. That's it. You can now enjoy !

Example
-------

Looking for "is_..." regexp in ngp source code.

![gif](/ngp.gif)

