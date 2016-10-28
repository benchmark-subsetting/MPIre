#include "mpire.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

//cpt_write is an integer incremented after a file was written
int cpt_write = 0;
int MPIre_active_dump; //0: Do not capture, 1: Capture
char MPIre_output_path[128]; //Where to save communication log

/** Create output directory "file_path" **/
int mkpath(char* file_path, mode_t mode) {
  assert(file_path && *file_path);
  char* p;
  for (p=strchr(file_path+1, '/'); p; p=strchr(p+1, '/')) {
    *p='\0';
    if (mkdir(file_path, mode)==-1) {
      if (errno!=EEXIST) { *p='/'; return -1; }
    }
    *p='/';
  }
  return 0;
}

/** Dump buffer "buf" **/
void write_in_file(void *buf, int count, MPI_Datatype datatype)
{
  MPI_File file;
  char filename[1024];
  int fp;

  //Dump file path
  snprintf(filename, sizeof(filename), "%s/%d", MPIre_output_path, cpt_write++);

  fp = MPI_File_open(MPI_COMM_SELF, filename, MPI_MODE_CREATE | MPI_MODE_RDWR, MPI_INFO_NULL, &file); 

  if(fp < 0) {
    fprintf(stderr, "Failed to open log file: %s\n", filename);
    exit(EXIT_FAILURE);
  }

  //Dump buffer to log file
  MPI_File_write(file, buf, count, datatype, MPI_STATUS_IGNORE);

  MPI_File_close(&file); 
}

void init_MPIre() {
  int rank, size;
  FILE* fp= NULL;
  char filename[128];
  struct stat info;

  char* mpire_rank = getenv("MPIRE_RANK");
  if(!mpire_rank) {
    fprintf(stderr, "Please export MPIRE_RANK=n where n is the MPI rank you want to capture\n");
    exit(EXIT_FAILURE);
  }
  MPIre_rank = atoi(mpire_rank);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  if(rank == MPIre_rank) {
    char* mpire_active_dump = getenv("MPIRE_ACTIVE_DUMP");
    if(!mpire_active_dump) {
      //If active dump is not defined set it to true
      setenv("MPIRE_ACTIVE_DUMP", "1", 1);
    }
    else MPIre_active_dump = atoi(mpire_active_dump);
    char* mpire_output_path = getenv("MPIRE_OUTPUT_PATH");
    if(!mpire_output_path) {
      snprintf(MPIre_output_path, sizeof(MPIre_output_path), ".mpire/dumps/%d/log/", MPIre_rank);
    }
    else snprintf(MPIre_output_path, sizeof(MPIre_output_path), "%s", mpire_output_path);
    htable_init(&requestHtab, rehash, NULL);

    //create directoty path
    if(stat(MPIre_output_path, &info) == -1)
      mkpath(MPIre_output_path, 0777);

    //Save how much processes are run during capture for later replay
    snprintf(filename, sizeof(filename), "%s/nprocs", MPIre_output_path);
    fp = fopen(filename, "w");
    if(fp==NULL) {
      fprintf(stderr, "Can't open file %s\n", filename);
      exit(EXIT_FAILURE);
    }
    //Get number of precesses
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    fprintf(fp, "%d", size);
    fclose(fp);
  }
}

/*
 ***** POINT TO POINT FUNCTIONS ***** 
 */
 
int MPI_Finalize( ) 
{
  int ret = PMPI_Finalize(); 
  htable_clear(&requestHtab);
  return ret;
}

void mpi_finalize_( int * ierror ) 
{
  *ierror = PMPI_Finalize(); 
  htable_clear(&requestHtab);
}

int MPI_Init( int * argc, char *** argv )
{
  int ret = PMPI_Init(argc, argv);
  init_MPIre();
  return ret;
}

void mpi_init_( int * ierror )
{
  *ierror = PMPI_Init(0, NULL);
  init_MPIre();
}

int MPI_Irecv( void * buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request * request )
{
  int rank, ret;
  MPI_Comm_rank(comm, &rank);
  if(rank == MPIre_rank && atoi(getenv("MPIRE_ACTIVE_DUMP")))
  {
    add_request(buf, count, datatype, request);
  }
  ret = PMPI_Irecv( buf, count, datatype, source, tag, comm, request );
  return ret;
}

void mpi_irecv_(MPI_Fint * buf, MPI_Fint * count, MPI_Fint * datatype, MPI_Fint * source, MPI_Fint * tag, MPI_Fint * comm, MPI_Fint * request, MPI_Fint * ierror)
{
  int rank;
  MPI_Datatype c_datatype = MPI_Type_f2c(*datatype);
  MPI_Comm c_comm = MPI_Comm_f2c(*comm);
  MPI_Request c_request = MPI_Request_f2c(*request);

  MPI_Comm_rank(c_comm, &rank);
  if(rank == MPIre_rank && atoi(getenv("MPIRE_ACTIVE_DUMP")))
  {
    add_request(buf, *count, c_datatype, request);
  }

  *ierror = PMPI_Irecv( buf, *count, c_datatype, *source, *tag, c_comm, &c_request );
  *request = MPI_Request_c2f(c_request); // output fortan
}

int MPI_Recv( void * buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status * status )
{
  int rank, ret;
  MPI_Comm_rank(comm, &rank);

  ret = PMPI_Recv( buf, count, datatype, source, tag, comm, status );

  if(rank == MPIre_rank && atoi(getenv("MPIRE_ACTIVE_DUMP"))) {
    write_in_file(buf, count, datatype);
  }
  return ret;
}

void mpi_recv_( MPI_Fint * buf, MPI_Fint * count, MPI_Fint * datatype, MPI_Fint * source, MPI_Fint * tag, MPI_Fint * comm, MPI_Fint * status , MPI_Fint * ierror )
{
  MPI_Datatype c_datatype = MPI_Type_f2c(*datatype);
  MPI_Comm c_comm = MPI_Comm_f2c(*comm);
  MPI_Status c_status;
  MPI_Status_f2c(status, &c_status);

  *ierror = MPI_Recv( buf, *count, c_datatype, *source, *tag, c_comm, &c_status );
  MPI_Status_c2f(&c_status, status); // output fortan
}

int MPI_Wait( MPI_Request * request, MPI_Status * status ) 
{
  int rank, ret;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  ret = PMPI_Wait(request, status);

  if(rank == MPIre_rank && atoi(getenv("MPIRE_ACTIVE_DUMP"))) 
  {
    request_t * newHtab = get_request(request);

    if(newHtab != NULL) {
      write_in_file(newHtab->buffer, newHtab->bufsize, newHtab->datatype);
      del_request(&requestHtab, hash_pointer(request, 0), newHtab);
    }
  }

  return ret;
}

void mpi_wait_( MPI_Fint * request, MPI_Fint * status, int * ierror )
{
  MPI_Request c_request = MPI_Request_f2c(*request);
  MPI_Status c_status;
  MPI_Status_f2c(status, &c_status);

  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  
  *ierror = PMPI_Wait(&c_request, &c_status);
  if(rank == MPIre_rank && atoi(getenv("MPIRE_ACTIVE_DUMP"))) 
  {
    request_t * newHtab = get_request(request);

    if(newHtab != NULL) {
      write_in_file(newHtab->buffer, newHtab->bufsize, newHtab->datatype);
      del_request(&requestHtab, hash_pointer(request, 0), newHtab);
    }
  }
  MPI_Status_c2f(&c_status, status); // output fortan
}

int MPI_Waitall( int count, MPI_Request *array_of_requests, MPI_Status * array_of_statuses )
{
  int rank, ret, i;
  //Maybe an issue here if the communicator is not MPI_COMM_WORLD
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  ret = PMPI_Waitall(count, array_of_requests, array_of_statuses);

  if(rank == MPIre_rank && atoi(getenv("MPIRE_ACTIVE_DUMP")))
  {
    request_t * newHtab = NULL;
    size_t hash_key[count];

    for(i = 0; i < count; i++)
    {
      newHtab = get_request(array_of_requests[i]);

      if(newHtab == NULL) continue;

      write_in_file(newHtab->buffer, newHtab->bufsize, newHtab->datatype);
      del_request(&requestHtab, hash_pointer(array_of_requests+i,0), newHtab);
    }
  }
  return ret;
}

void mpi_waitall_( MPI_Fint * count, MPI_Fint * array_of_requests, MPI_Fint * array_of_statuses, int * ierror ) 
{
  int rank, i;
  MPI_Request c_request[*count];
  MPI_Status c_status[*count];
  //Maybe an issue here if the communicator is not MPI_COMM_WORLD
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  for(i = 0; i < *count; i++) {
    c_request[i] = MPI_Request_f2c(array_of_requests[i]);
    MPI_Status_f2c(&(array_of_statuses[i]), &c_status[i]);
  }

  *ierror = PMPI_Waitall(*count, c_request, c_status);

  if(rank == MPIre_rank && atoi(getenv("MPIRE_ACTIVE_DUMP")))
  {
    request_t * newHtab = NULL;
    size_t hash_key[*count];

    for(i = 0; i < *count; i++)
    {
      newHtab = get_request(array_of_requests+i);

      //Request can be for rcv or send, if the request is not registered in the
      //htable it means this request was for a send.
      if(newHtab == NULL) {
        continue;
      }

      write_in_file(newHtab->buffer, newHtab->bufsize, newHtab->datatype);
      del_request(&requestHtab, hash_pointer(array_of_requests+i,0), newHtab);
    }
  }
  for(i = 0; i < *count; i++) {
    MPI_Status_c2f(&c_status[i], &array_of_statuses[i]); // output fortan
  }
}

int MPI_Bcast( void * buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm ) 
{
  int rank, ret;
  MPI_Comm_rank(comm, &rank);

  ret = PMPI_Bcast( buffer, count, datatype, root, comm );

  if(rank == MPIre_rank && rank != root && atoi(getenv("MPIRE_ACTIVE_DUMP"))) {
    write_in_file(buffer, count, datatype);
  }

  return ret;
}

void mpi_bcast_( MPI_Fint * buffer, MPI_Fint * count, MPI_Fint * datatype, MPI_Fint * root, MPI_Fint * comm, MPI_Fint * ierror ) 
{
  MPI_Datatype c_datatype = MPI_Type_f2c(*datatype);
  MPI_Comm c_comm = MPI_Comm_f2c(*comm);

  *ierror = MPI_Bcast( buffer, *count, c_datatype, *root, c_comm );
}

int MPI_Alltoall( const void * sendbuf, int sendcount, MPI_Datatype sendtype, void * recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm ) {
  int rank, ret, size;

  MPI_Comm_size(comm, &size);
  MPI_Comm_rank(comm, &rank);

  ret = PMPI_Alltoall( sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
  if(rank == MPIre_rank && atoi(getenv("MPIRE_ACTIVE_DUMP")))
    write_in_file(recvbuf, recvcount*size, recvtype);
  return ret;
}

void mpi_alltoall_( MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierror ) {
  MPI_Comm c_comm = MPI_Comm_f2c(*comm);
  MPI_Datatype c_sendtype = MPI_Type_f2c(*sendtype);
  MPI_Datatype c_recvtype = MPI_Type_f2c(*recvtype);
  *ierror = MPI_Alltoall(sendbuf, *sendcount, c_sendtype, recvbuf, *recvcount, c_recvtype, c_comm);
}


int MPI_Alltoallv( const void * sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype, void * recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm ) {
  int rank, ret, i, size;
  int total_recvcounts=0;

  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &size);
  ret = PMPI_Alltoallv( sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm);
  if(rank == MPIre_rank && atoi(getenv("MPIRE_ACTIVE_DUMP"))) {
    //Count total elements to be received
    for (i = 0; i < size; i++)
      total_recvcounts += recvcounts[i];
    write_in_file(recvbuf, total_recvcounts, recvtype);
  }
  return ret;
}

void mpi_alltoallv_( MPI_Fint *sendbuf, MPI_Fint *sendcounts[], MPI_Fint *sdispls[], MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts[], MPI_Fint *rdispls[], MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint * ierror ) {
  MPI_Comm c_comm = MPI_Comm_f2c(*comm);
  MPI_Datatype c_sendtype = MPI_Type_f2c(*sendtype);
  MPI_Datatype c_recvtype = MPI_Type_f2c(*recvtype);
  *ierror = MPI_Alltoallv(sendbuf, *sendcounts, *sdispls, c_sendtype, recvbuf, *recvcounts, *rdispls, c_recvtype, c_comm);
}

int MPI_Allreduce( const void * sendbuf, void * recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm ) {
  int rank, ret;
  MPI_Comm_rank(comm, &rank);
  ret = PMPI_Allreduce( sendbuf, recvbuf, count, datatype, op, comm );
  if(rank == MPIre_rank && atoi(getenv("MPIRE_ACTIVE_DUMP"))) {
    write_in_file(recvbuf, count, datatype);
  }
  return ret;
}

void mpi_allreduce_( MPI_Fint * sendbuf, MPI_Fint * recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint * ierror ) {
  MPI_Comm c_comm = MPI_Comm_f2c(*comm);
  MPI_Datatype c_datatype = MPI_Type_f2c(*datatype);
  MPI_Op c_op = MPI_Op_f2c(*op);

  *ierror = MPI_Allreduce(sendbuf, recvbuf, *count, c_datatype, c_op, c_comm);
}

int MPI_Reduce( const void * sendbuf, void * recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm ) {
  int rank, ret;
  MPI_Comm_rank(comm, &rank);
  ret = PMPI_Reduce( sendbuf, recvbuf, count, datatype, op, root, comm );
  if(rank == MPIre_rank && rank == root && atoi(getenv("MPIRE_ACTIVE_DUMP"))) {
    write_in_file(recvbuf, count, datatype);
  }
  return ret;
}
void mpi_reduce_( MPI_Fint * sendbuf, MPI_Fint * recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *root, MPI_Fint *comm, MPI_Fint * ierror ) {
  MPI_Comm c_comm = MPI_Comm_f2c(*comm);
  MPI_Datatype c_datatype = MPI_Type_f2c(*datatype);
  MPI_Op c_op = MPI_Op_f2c(*op);
  *ierror = MPI_Reduce(sendbuf, recvbuf, *count, c_datatype, c_op, *root, c_comm);
}

/* Non implemented MPI Hooks */
void non_implemented_hook() {
  abort();
}

void non_implemented_hook_special( const char * caller_name )
{
  fprintf(stderr, "Function <%s> not implemented yet.\n", caller_name);
  non_implemented_hook();
}

#define non_implemented_hook(void) non_implemented_hook_special(__func__)

int MPI_Ireduce( const void * sendbuf, void * recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPI_Request * request ) 
{ non_implemented_hook(); }
int MPI_Iallreduce( const void * sendbuf, void * recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request * request ) 
{ non_implemented_hook(); }
int MPI_Reduce_scatter( const void * sendbuf, void * recvbuf, const int recvcounts[], MPI_Datatype datatype, MPI_Op op, MPI_Comm comm ) 
{ non_implemented_hook(); }
int MPI_Ireduce_scatter( const void * sendbuf, void * recvbuf, const int recvcounts[], MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request * request ) 
{ non_implemented_hook(); }
int MPI_Reduce_scatter_block( const void * sendbuf, void * recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm ) 
{ non_implemented_hook(); }
int MPI_Ireduce_scatter_block( const void * sendbuf, void * recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request * request ) 
{ non_implemented_hook(); }
int MPI_Exscan( const void * sendbuf, void * recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm ) 
{ non_implemented_hook(); }
int MPI_Iexscan( const void * sendbuf, void * recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request * request ) 
{ non_implemented_hook(); }
int MPI_Scan( const void * sendbuf, void * recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm ) 
{ non_implemented_hook(); }
int MPI_Iscan( const void * sendbuf, void * recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request * request ) 
{ non_implemented_hook(); }
int MPI_Allgather( const void * sendbuf, int sendcount, MPI_Datatype sendtype, void * recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm ) 
{ non_implemented_hook(); }
int MPI_Iallgather( const void * sendbuf, int sendcount, MPI_Datatype sendtype, void * recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request * request ) 
{ non_implemented_hook(); }
int MPI_Allgatherv( const void * sendbuf, int sendcount, MPI_Datatype sendtype, void * recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, MPI_Comm comm ) 
{ non_implemented_hook(); }
int MPI_Iallgatherv( const void * sendbuf, int sendcount, MPI_Datatype sendtype, void * recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request * request ) 
{ non_implemented_hook(); }
int MPI_Ialltoall( const void * sendbuf, int sendcount, MPI_Datatype sendtype, void * recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request * request )
{ non_implemented_hook(); }
int MPI_Ialltoallv( const void * sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype, void * recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request * request ) 
{ non_implemented_hook(); }
int MPI_Alltoallw( const void * sendbuf, const int sendcounts[], const int sdispls[], const MPI_Datatype sendtypes[], void * recvbuf, const int recvcounts[], const int rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm ) 
{ non_implemented_hook(); }
int MPI_Ialltoallw( const void * sendbuf, const int sendcounts[], const int sdispls[], const MPI_Datatype sendtypes[], void * recvbuf, const int recvcounts[], const int rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm, MPI_Request * request ) 
{ non_implemented_hook(); }
int MPI_Recv_init( void * buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request * request )
{ non_implemented_hook(); }
void mpi_recv_init_( MPI_Fint * buf, MPI_Fint * count, MPI_Fint * datatype, MPI_Fint * source, MPI_Fint * tag, MPI_Fint * comm, MPI_Fint * request , MPI_Fint * ierror )
{ non_implemented_hook(); }
int MPI_Sendrecv( const void * sendbuf, int sendcount, MPI_Datatype sendtype, int dest, int sendtag, void * recvbuf, int recvcount, MPI_Datatype recvtype, int source, int recvtag, MPI_Comm comm, MPI_Status * status )
{ non_implemented_hook(); }
void mpi_sendrecv_( MPI_Fint * sendbuf, MPI_Fint * sendcount, MPI_Fint * sendtype, MPI_Fint * dest, MPI_Fint * sendtag, MPI_Fint * recvbuf, MPI_Fint * recvcount, MPI_Fint * recvtype, MPI_Fint * source, MPI_Fint * recvtag, MPI_Fint * comm, MPI_Fint * status , MPI_Fint * ierror )
{ non_implemented_hook(); }
int MPI_Sendrecv_replace( void * buf, int count, MPI_Datatype datatype, int dest, int sendtag, int source, int recvtag, MPI_Comm comm, MPI_Status * status )
{ non_implemented_hook(); }
void mpi_sendrecv_replace_( MPI_Fint * buf, MPI_Fint * count, MPI_Fint * datatype, MPI_Fint * dest, MPI_Fint * sendtag, MPI_Fint * source, MPI_Fint * recvtag, MPI_Fint * comm, MPI_Fint * status , MPI_Fint * ierror )
{ non_implemented_hook(); }
int MPI_Ibcast( void * buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPI_Request * request )
{ non_implemented_hook(); }
void mpi_ibcast_( MPI_Fint * buffer, MPI_Fint * count, MPI_Fint * datatype, MPI_Fint * root, MPI_Fint * comm, MPI_Fint * request, MPI_Fint * ierror ) 
{ non_implemented_hook(); }
int MPI_Gather( const void * sendbuf, int sendcount, MPI_Datatype sendtype, void * recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm ) 
{ non_implemented_hook(); }
void mpi_gather_( MPI_Fint * sendbuf, MPI_Fint * sendcount, MPI_Fint * sendtype, MPI_Fint * recvbuf, MPI_Fint * recvcount, MPI_Fint * recvtype, MPI_Fint * root, MPI_Fint * comm, MPI_Fint * ierror ) 
{ non_implemented_hook(); }
int MPI_Igather( const void * sendbuf, int sendcount, MPI_Datatype sendtype, void * recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request * request ) 
{ non_implemented_hook(); }
void mpi_igather_( MPI_Fint * sendbuf, MPI_Fint * sendcount, MPI_Fint * sendtype, MPI_Fint * recvbuf, MPI_Fint * recvcount, MPI_Fint * recvtype, MPI_Fint * root, MPI_Fint * comm, MPI_Fint * request, MPI_Fint * ierror ) 
{ non_implemented_hook(); }
int MPI_Gatherv( const void * sendbuf, int sendcount, MPI_Datatype sendtype, void * recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, int root, MPI_Comm comm )
{ non_implemented_hook(); }
void mpi_gatherv_( MPI_Fint * sendbuf, MPI_Fint * sendcount, MPI_Fint * sendtype, MPI_Fint * recvbuf, MPI_Fint * recvcounts, MPI_Fint * displs, MPI_Fint * recvtype, MPI_Fint * root, MPI_Fint * comm, MPI_Fint * request, MPI_Fint * ierror ) 
{ non_implemented_hook(); }
int MPI_Igatherv( const void * sendbuf, int sendcount, MPI_Datatype sendtype, void * recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request * request )
{ non_implemented_hook(); }
void mpi_igatherv_( MPI_Fint * sendbuf, MPI_Fint * sendcount, MPI_Fint * sendtype, MPI_Fint * recvbuf, MPI_Fint * recvcounts, MPI_Fint * displs, MPI_Fint * recvtype, MPI_Fint * root, MPI_Fint * comm, MPI_Fint * request, MPI_Fint * ierror ) 
{ non_implemented_hook(); }
int MPI_Scatter( const void * sendbuf, int sendcount, MPI_Datatype sendtype, void * recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm ) 
{ non_implemented_hook(); }
void mpi_scatter_( MPI_Fint * sendbuf, MPI_Fint * sendcount, MPI_Fint * sendtype, MPI_Fint * recvbuf, MPI_Fint * recvcount, MPI_Fint * recvtype, MPI_Fint * root, MPI_Fint * comm, MPI_Fint * ierror ) 
{ non_implemented_hook(); }
int MPI_Iscatter( const void * sendbuf, int sendcount, MPI_Datatype sendtype, void * recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request * request ) 
{ non_implemented_hook(); }
void mpi_iscatter_( MPI_Fint * sendbuf, MPI_Fint * sendcount, MPI_Fint * sendtype, MPI_Fint * recvbuf, MPI_Fint * recvcount, MPI_Fint * recvtype, MPI_Fint * root, MPI_Fint * comm, MPI_Fint * request, MPI_Fint * ierror ) 
{ non_implemented_hook(); }
int MPI_Scatterv( const void * sendbuf, const int sendcounts[], const int displs[], MPI_Datatype sendtype, void * recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm ) 
{ non_implemented_hook(); }
void mpi_scatterv_( MPI_Fint * sendbuf, MPI_Fint * sendcounts[], MPI_Fint * displs[], MPI_Fint * sendtype, MPI_Fint * recvbuf, MPI_Fint * recvcount, MPI_Fint * recvtype, MPI_Fint * root, MPI_Fint * comm, MPI_Fint * ierror ) 
{ non_implemented_hook(); }
int MPI_Iscatterv( const void * sendbuf, const int sendcounts[], const int displs[], MPI_Datatype sendtype, void * recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request * request )
{ non_implemented_hook(); }
void mpi_iscatterv_( MPI_Fint * sendbuf, MPI_Fint * sendcounts[], MPI_Fint * displs[], MPI_Fint * sendtype, MPI_Fint * recvbuf, MPI_Fint * recvcount, MPI_Fint * recvtype, MPI_Fint * root, MPI_Fint * comm, MPI_Fint * request, MPI_Fint * ierror )
{ non_implemented_hook(); }
