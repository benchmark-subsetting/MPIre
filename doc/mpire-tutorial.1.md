MPIre tutorial(1) -- Example on how to use MPIre
====================================================================

## DESCRIPTION

MPIre is an open source library that allows the user to replay any MPI rank
from MPI applications. It first saves communications for the desired rank during a
capture run. Then it uses the log files to simulate communications during the replay run.

In this tutorial we will use MPIre to replay rank 0 for the Integer Sort (IS)
from the [NAS-MPI 3.3 benchmarks](http://www.nas.nasa.gov/).

## RANK 0 CAPTURE

Inside the IS directory build the application as you would normally do for A
class and 4 MPI ranks with

```bash
$ make CLASS=A NPROCS=4
```

Choose the rank you want to capture (i.e. rank 0)

```bash
$ export MPIRE_RANK=0
```

Then you have to run IS while MPIre captures the desired rank with `libmpire_capture`.
To use MPIre libmpire_capture you should define the LD_PRELOAD environment variable, 

```bash
$ LD_PRELOAD=/usr/local/lib/libmpire_capture.so mpirun -n 4 ./is.A.4
```

By default capture logs files are saved in ".mpire/dumps/<rank>/log/".

## RANK 0 REPLAY

Now you can replay the rank 0 by using `libmpire_replay`,

```bash
$ LD_PRELOAD=/usr/local/lib/libmpire_replay.so ./is.A.4
```

As you can see you don't need to run it with mpirun since it only executes
one process. Still, MPIre makes sure that the initial MPI environment is restored. It
means that for instance, MPI_Comm_size will return the inital number of MPI ranks
at capture time, here 4.

## OPTIONAL PARAMETERS

By default logs files are saved in ".mpire/dumps/<rank>/log/". You can modify
this path with the following command

```bash
$ export MPIRE_OUTPUT_PATH="your/path"
```

Also, MPIre captures MPI communications since the start of the application. You
can modify this behaviour by setting the MPIRE_ACTIVE_DUMP variable to 0. MPIre
won't capture until you set it back to 1.

```bash
$ export MPIRE_ACTIVE_DUMP=0
```

## COPYRIGHT

MPIre is Copyright (C) 2016 Université de Versailles St-Quentin-en-Yvelines
