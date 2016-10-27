### Installation

Please ensure that MPIre's dependencies are installed on your system:

  * OpenMPI

Then run the following command inside MPIre directory:

```bash
   $ ./autogen.sh
   $ ./configure
   $ make
   $ make install
```
It installs two libraries, libmpire_capture and libmpire_replay inside
/usr/local/lib/ that are used to either capture or replay any MPI rank.

Once installation is over, we recommend that you run the test suite to ensure
MPIre works as expected on your system:

```bash
   $ make check
```
