# MPIre v0.0.1

[![Build Status](https://travis-ci.org/benchmark-subsetting/mpire.svg?branch=master)](https://travis-ci.org/benchmark-subsetting/MPIre)

[MPI Replayer
(MPIre)](https://benchmark-subsetting.github.io/MPIre/) is an open source library
that allows the user to replay any MPI rank from MPI applications. It first saves
communication for the desired rank at capture run. Then it uses log file to simulate
communication at replay run.

### Installation

Please follow the instructions in
[INSTALL.md](https://github.com/benchmark-subsetting/MPIre/blob/master/INSTALL.md).

### Supported platforms

For now MPIre only supports the Linux operating system. Mpire has been tested
mainly on x86_64 Debian and Ubuntu distributions with OpenMPI 1.10.2 which
implements MPI 3.0.

MPIre is an alpha release, if you experience bugs during capture and
replay please report them using the issue tracker. Thanks !

### Documentation

New users should start by reading [MPIre
tutorial](https://github.com/benchmark-subsetting/MPÏre/blob/master/doc/mpire-tutorial.1.md).

Once installation is complete, a set of man pages for MPIre is available
in the `doc/` directory. To check it use

```bash
man -M doc/ mpire-tutorial
```

### Limitations

Communication logs are saved in the chronological order and replayed in the same
order. MPIre then make the assumption that the execution is deterministic, and
won't work for non deterministic code like program based on random number or
code where the order of communication can change.

Also some MPI communication functions are not yet wrapped into MPIre. It means
that MPIre will fail and exit if it encounter such functions.

### Bugs, Feedback and Contributions

The issue tracker is the preferred channel for bug reports, features requests and
submitting pull requests.

For more general questions or discussions please use
[cere-dev@googlegroups.com mailing
list](https://groups.google.com/forum/#!forum/cere-dev).

### Contributors

MPIre contributors are listed in the [THANKS
file](https://github.com/benchmark-subsetting/MPIre/blob/master/THANKS).

### License and copyright

Copyright (c) 2013-2016, Universite de Versailles St-Quentin-en-Yvelines

MPIre is free software: you can redistribute it and/or modify it under the terms of
the GNU Lesser General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.
