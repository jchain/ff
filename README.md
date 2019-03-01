# ff

Simplified version of *find* using the PCRE library for regular expressions.

*ff* has been inspired by [*fd*](https://github.com/sharkdp/fd).

The problem with [*fd*](https://github.com/sharkdp/fd) is that it is
written in Rust and not everyone is willing to download nearly 1GB of
compiler for something that simple.  *ff* on the other hand is written
in C and can be compiled with the C compiler that already comes with
your POSIX system.

## Features

- Convenient syntax: `ff PATTERN` instead of `find -iname '*PATTERN*'`
- Ignores hidden directories and files, by default
- Regular expressions

## Future features (hopefully)

- Parallel directory traversal
- Colorized output
- Command execution
- Exclude files and directories
