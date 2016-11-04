# MPIre v0.0.1

[![Build Status](https://travis-ci.org/benchmark-subsetting/MPIre.svg?branch=master)](https://travis-ci.org/benchmark-subsetting/MPIre)

MPIre stands for MPI replayer. MPIre is an open source library that allows the user to capture and replay a single rank 
inside a whole MPI application. MPIre allows to simulate the communications of the other ranks without running them. 
It is useful to debug performance or crashes in a single node without having to launch all the original MPI processes. 
During the capture phase, MPIre intercepts and saves all the inbound communication for the desired rank to a set of capture 
log files. Then, at replay, MPIre replaces the standard MPI calls with reads to the logs to simulate the communications 
received from the other ranks.

### Installation

Please follow the instructions in
[INSTALL.md](https://github.com/benchmark-subsetting/MPIre/blob/master/INSTALL.md).

### Supported platforms

Current MPIre version only supports the Linux operating system. MPIre has been tested
on x86_64 Debian and Ubuntu distributions with OpenMPI 1.10.2 which implements MPI 3.0.

MPIre is an alpha release, if you experience bugs during capture and replay please report 
them using the issue tracker. Thanks !

### Documentation

New users should start by reading [MPIre
tutorial](https://github.com/benchmark-subsetting/MPIre/blob/master/doc/mpire-tutorial.1.md).

Once installation is complete, a set of man pages for MPIre is available
in the `doc/` directory. To check it use

```bash
man -M doc/ mpire-tutorial
```

### Limitations

Communication logs are saved in the chronological order and replayed in the same
order. MPIre makes the assumption that the execution is deterministic, and
won't work for non deterministic code such as programs where the order of communication 
can change from one execution to the next (eg. programs performing dynamic scheduling or work-balance).

Some MPI communication functions are not yet wrapped into MPIre. MPIre will fail 
and exit with an appropriate error message if it reaches non-supported functions.

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

Copyright (c) 2016, Universite de Versailles St-Quentin-en-Yvelines

MPIre is free software: you can redistribute it and/or modify it under the terms of
the GNU Lesser General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.
