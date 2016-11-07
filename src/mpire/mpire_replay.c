#include "mpire.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

/*
 * cpt_read is an integer incremented after a file was read
 */
int cpt_read = 0;
char MPIre_input_path[128]; //Where to save communication log
int MPIre_size;

/** Load buffer "buf" **/
void read_in_file(void *buf, int count, MPI_Datatype datatype)
{
  MPI_File file;
  int fp, i;
  char filename[254];

  //Log file path
  snprintf(filename, sizeof(filename), "%s/%d", MPIre_input_path, cpt_read++);

  fp = MPI_File_open(MPI_COMM_SELF, filename, MPI_MODE_RDONLY, MPI_INFO_NULL, &file); 

  if(fp != MPI_SUCCESS)
  {
    fprintf(stderr, "Failed to open log file: %s\n", filename);
    fprintf(stderr, "Set correct log path with \"export MPIRE_INPUT_PATH=<log_path>\"");
    exit(EXIT_FAILURE);
  }
  fprintf(stderr, "Opened log file %s\n", filename);
  //Load buffer from file into buf
  int my_read_error = MPI_File_read(file, buf, count, datatype, MPI_STATUS_IGNORE);

  MPI_File_close(&file);
}

void init_MPIre() {
  int rank;
  FILE* fp;
  char filename[128];

  char* mpire_rank = getenv("MPIRE_RANK");
  if(!mpire_rank) {
    fprintf(stderr, "Please export MPIRE_RANK=n where n is the MPI rank you want to replay\n");
    exit(EXIT_FAILURE);
  }
  MPIre_rank = atoi(mpire_rank);

  char* mpire_input_path = getenv("MPIRE_INPUT_PATH");
  if(!mpire_input_path) {
    snprintf(MPIre_input_path, sizeof(MPIre_input_path), ".mpire/dumps/%d/log/", MPIre_rank);
  }
  else snprintf(MPIre_input_path, sizeof(MPIre_input_path), "%s", mpire_input_path);
  htable_init(&requestHtab, rehash, NULL);

  //Load original number of precesses at capture
  snprintf(filename, sizeof(filename), "%s/nprocs", MPIre_input_path);
  fp = fopen(filename, "r");
  if(fp==NULL) {
    fprintf(stderr, "Can't open file %s\n", filename);
    exit(EXIT_FAILURE);
  }
  int res = fscanf(fp, "%d", &MPIre_size);
  fclose(fp);
}

int MPI_Init( int * argc, char *** argv ) {
  #ifdef DEBUG
    print_calling_function();
  #endif
  int init;
  MPI_Initialized(&init);
  if(init) {
    cpt_read = 0;
    return MPI_SUCCESS;
  }
  else {
    init_MPIre();
    return PMPI_Init( argc, argv );
  }
}

void mpi_init_( int * ierror ) {
  #ifdef DEBUG
    print_calling_function();
  #endif
  int init;
  MPI_Initialized(&init);
  if(init) {
    cpt_read = 0;
    *ierror = MPI_SUCCESS;
  }
  else {
    init_MPIre();
    *ierror = PMPI_Init(0, NULL);
  }
}

//~int MPI_Initialized( int * flag ) {*flag=1; return MPI_SUCCESS;}
//~void mpi_initialized_( int * flag, int * ierror ) {*flag=1; *ierror = MPI_SUCCESS;}

int MPI_Finalize( ) {
  #ifdef DEBUG
    print_calling_function();
  #endif
  int ret = PMPI_Finalize();
  htable_clear(&requestHtab);
  return ret;
}

void mpi_finalize_( int * ierror ) {
  *ierror = MPI_Finalize();
}

int MPI_Finalized( int * flag ) {*flag=1;return MPI_SUCCESS;}
void mpi_finalized_( int * flag, int * ierror ) {*flag=1;*ierror = MPI_SUCCESS;}

int MPI_Irecv( void * buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request * request )
{
  #ifdef DEBUG
    print_calling_function();
  #endif
  add_request(buf, count, datatype, request);

  return MPI_SUCCESS;
}

void mpi_irecv_(MPI_Fint * buf, MPI_Fint * count, MPI_Fint * datatype, MPI_Fint * source, MPI_Fint * tag, MPI_Fint * comm, MPI_Fint * request, MPI_Fint * ierror)
{
  #ifdef DEBUG
    print_calling_function();
  #endif
  MPI_Comm c_comm = MPI_Comm_f2c(*comm);
  MPI_Datatype c_datatype = MPI_Type_f2c(*datatype);

  add_request(buf, *count, c_datatype, request);

  *ierror = MPI_SUCCESS;
}

int MPI_Recv( void * buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status * status )
{
  #ifdef DEBUG
    print_calling_function();
  #endif
  read_in_file(buf, count, datatype);

  return MPI_SUCCESS;
}

void mpi_recv_( void * buf, MPI_Fint * count, MPI_Fint * datatype, MPI_Fint * source, MPI_Fint * tag, MPI_Fint * comm, MPI_Fint * status , MPI_Fint * ierror )
{
  MPI_Datatype c_datatype = MPI_Type_f2c(*datatype);
  MPI_Comm c_comm = MPI_Comm_f2c(*comm);
  MPI_Status c_status;
  MPI_Status_f2c(status, &c_status);

  *ierror = MPI_Recv( buf, *count, c_datatype, *source, *tag, c_comm, &c_status );
}

//~int MPI_Sendrecv( const void * sendbuf, int sendcount, MPI_Datatype sendtype, int dest, int sendtag, void * recvbuf, int recvcount, MPI_Datatype recvtype, int source, int recvtag, MPI_Comm comm, MPI_Status * status ) {}
//~void mpi_sendrecv_( void * sendbuf, int * sendcount, int * sendtype, int * dest, int * sendtag, void * recvbuf, int * recvcount, int * recvtype, int * source, int * recvtag, int * comm, const int * status , int * ierror ) {}
//~int MPI_Sendrecv_replace( void * buf, int count, MPI_Datatype datatype, int dest, int sendtag, int source, int recvtag, MPI_Comm comm, MPI_Status * status ) {}
//~void mpi_sendrecv_replace_( void * buf, int * count, int * datatype, int * dest, int * sendtag, int * source, int * recvtag, int * comm, const int * status , int * ierror ) {}
//~MPI_Comm MPI_Comm_f2c(MPI_Fint comm){return 0;}
//~MPI_File MPI_File_f2c(MPI_Fint file){return 0;}
//~MPI_Group MPI_Group_f2c(MPI_Fint group){return 0;}
//~MPI_Info MPI_Info_f2c(MPI_Fint info){return 0;}
//~MPI_Op MPI_Op_f2c(MPI_Fint op){return 0;}
//~MPI_Request MPI_Request_f2c(MPI_Fint request){return 0;}
//~MPI_Datatype MPI_Type_f2c(MPI_Fint datatype){return 0;}
//~MPI_Win MPI_Win_f2c(MPI_Fint win){return 0;}
int MPI_Abort( MPI_Comm comm, int errorcode ) {return MPI_SUCCESS;}
void mpi_abort_( MPI_Comm comm, int errorcode, int * ierror ) {*ierror = MPI_SUCCESS;}
//~int MPI_Accumulate( const void * origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win ) {return MPI_SUCCESS;}
//~void mpi_accumulate_( const void * origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win, int * ierror ) {*ierror = MPI_SUCCESS;}
//~int MPI_Add_error_class( int * errorclass ) {return MPI_SUCCESS;}
//~void mpi_add_error_class_( int * errorclass, int * ierror ) {*ierror = MPI_SUCCESS;}
//~int MPI_Add_error_code( int errorclass, int * errorcode ) {return MPI_SUCCESS;}
//~void mpi_add_error_code_( int errorclass, int * errorcode, int * ierror ) {*ierror = MPI_SUCCESS;}
//~int MPI_Add_error_string( int errorcode, const char * string ) {return MPI_SUCCESS;}
//~void mpi_add_error_string_( int errorcode, const char * string, int * ierror ) {*ierror = MPI_SUCCESS;}
//~int MPI_Address( void * location, MPI_Aint * address ) {return MPI_SUCCESS;}
//~void mpi_address_( void * location, MPI_Aint * address, int * ierror ) {*ierror = MPI_SUCCESS;}
//~int MPI_Allgather( const void * sendbuf, int sendcount, MPI_Datatype sendtype, void * recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm ) {return MPI_SUCCESS;}
//~int MPI_Iallgather( const void * sendbuf, int sendcount, MPI_Datatype sendtype, void * recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request * request ) {return MPI_SUCCESS;}
//~int MPI_Allgatherv( const void * sendbuf, int sendcount, MPI_Datatype sendtype, void * recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, MPI_Comm comm ) {return MPI_SUCCESS;}
//~int MPI_Iallgatherv( const void * sendbuf, int sendcount, MPI_Datatype sendtype, void * recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request * request ) {return MPI_SUCCESS;}
//~int MPI_Alloc_mem( MPI_Aint size, MPI_Info info, void * baseptr ) {return MPI_SUCCESS;}
//~void mpi_alloc_mem_( MPI_Aint size, MPI_Info info, void * baseptr, int * ierror ) {*ierror = MPI_SUCCESS;}
//~int MPI_Iallreduce( const void * sendbuf, void * recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request * request ) {return MPI_SUCCESS;}
//~int MPI_Ialltoall( const void * sendbuf, int sendcount, MPI_Datatype sendtype, void * recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request * request ) {return MPI_SUCCESS;}

int MPI_Allreduce( const void * sendbuf, void * recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm ) {
  #ifdef DEBUG
    print_calling_function();
  #endif
  read_in_file(recvbuf, count, datatype);

  return MPI_SUCCESS;
}

void mpi_allreduce_( MPI_Fint * sendbuf, MPI_Fint * recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint * ierror ) {
  MPI_Datatype c_datatype = MPI_Type_f2c(*datatype);
  MPI_Comm c_comm = MPI_Comm_f2c(*comm);
  MPI_Op c_op = MPI_Op_f2c(*op);

  *ierror = MPI_Allreduce(sendbuf, recvbuf, *count, c_datatype, c_op, c_comm);
}

int MPI_Alltoall( const void * sendbuf, int sendcount, MPI_Datatype sendtype, void * recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm ) {
  #ifdef DEBUG
    print_calling_function();
  #endif
  int size;
  MPI_Comm_size(comm, &size);

  read_in_file(recvbuf, recvcount*size, recvtype);

  return MPI_SUCCESS;
}

void mpi_alltoall_( MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierror ) {
  MPI_Comm c_comm = MPI_Comm_f2c(*comm);
  MPI_Datatype c_sendtype = MPI_Type_f2c(*sendtype);
  MPI_Datatype c_recvtype = MPI_Type_f2c(*recvtype);
  *ierror = MPI_Alltoall(sendbuf, *sendcount, c_sendtype, recvbuf, *recvcount, c_recvtype, c_comm);
}

int MPI_Alltoallv( const void * sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype, void * recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm ) {
  #ifdef DEBUG
    print_calling_function();
  #endif
  int size, i;
  int total_recvcounts=0;

  MPI_Comm_size(comm, &size);

  //Count total elements to be received
  for (i = 0; i < size; i++)
    total_recvcounts += recvcounts[i];
  read_in_file(recvbuf, total_recvcounts, recvtype);

  return MPI_SUCCESS;
}

void mpi_alltoallv_( MPI_Fint *sendbuf, MPI_Fint *sendcounts[], MPI_Fint *sdispls[], MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts[], MPI_Fint *rdispls[], MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierror ) {
  MPI_Comm c_comm = MPI_Comm_f2c(*comm);
  MPI_Datatype c_sendtype = MPI_Type_f2c(*sendtype);
  MPI_Datatype c_recvtype = MPI_Type_f2c(*recvtype);
  *ierror = MPI_Alltoallv(sendbuf, *sendcounts, *sdispls, c_sendtype, recvbuf, *recvcounts, *rdispls, c_recvtype, c_comm);
}

//~int MPI_Ialltoallv( const void * sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype, void * recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request * request ) {return MPI_SUCCESS;}
//~int MPI_Alltoallw( const void * sendbuf, const int sendcounts[], const int sdispls[], const MPI_Datatype sendtypes[], void * recvbuf, const int recvcounts[], const int rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm ) {return MPI_SUCCESS;}
//~int MPI_Ialltoallw( const void * sendbuf, const int sendcounts[], const int sdispls[], const MPI_Datatype sendtypes[], void * recvbuf, const int recvcounts[], const int rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm, MPI_Request * request ) {return MPI_SUCCESS;}*/
//~int MPI_Attr_delete( MPI_Comm comm, int keyval ) {return MPI_SUCCESS;}
//~void mpi_attr_delete_( MPI_Comm comm, int keyval, int * ierror ) {*ierror = MPI_SUCCESS;}
//~int MPI_Attr_get( MPI_Comm comm, int keyval, void * attribute_val, int * flag ) {return MPI_SUCCESS;}
//~void mpi_attr_get_( MPI_Comm comm, int keyval, void * attribute_val, int * flag, int * ierror ) {*ierror = MPI_SUCCESS;}
//~int MPI_Attr_put( MPI_Comm comm, int keyval, void * attribute_val ) {return MPI_SUCCESS;}
//~void mpi_attr_put_( MPI_Comm comm, int keyval, void * attribute_val, int * ierror ) {*ierror = MPI_SUCCESS;}

int MPI_Barrier( MPI_Comm comm ) {return MPI_SUCCESS;}
void mpi_barrier_( MPI_Fint *comm, MPI_Fint *ierror ) {*ierror = MPI_SUCCESS;}

//~ int MPI_Ibarrier( MPI_Comm comm, MPI_Request * request ) {return MPI_SUCCESS;}
//~ void mpi_ibarrier_( MPI_Comm comm, MPI_Request * request, int * ierror ) {*ierror = MPI_SUCCESS;}

int MPI_Bcast(void * buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm) {
  #ifdef DEBUG
    print_calling_function();
  #endif
  if(MPIre_rank != root) {
    read_in_file(buffer, count, datatype);
  }
  return MPI_SUCCESS;
}

void mpi_bcast_( MPI_Fint *buffer, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror ) {
  MPI_Datatype c_datatype = MPI_Type_f2c(*datatype);
  MPI_Comm c_comm = MPI_Comm_f2c(*comm);
  *ierror = MPI_Bcast( buffer, *count, c_datatype, *root, c_comm );
}

int MPI_Waitall( int count, MPI_Request array_of_requests[], MPI_Status * array_of_statuses ) {
  #ifdef DEBUG
    print_calling_function();
  #endif
  int i;

  request_t * newHtab = NULL;

  for(i = 0; i < count; i++)
  {
    newHtab = get_request(array_of_requests+i);

    //Request can be for rcv or send, if the request is not registered in the
    //htable it means this request was for a send.
    if(newHtab == NULL) continue;

    read_in_file(newHtab->buffer, newHtab->bufsize, newHtab->datatype);
    del_request(&requestHtab, hash_pointer(array_of_requests+i,0), newHtab);
  }

  return MPI_SUCCESS;
}

void mpi_waitall_( MPI_Fint *count, MPI_Fint *array_of_requests, MPI_Fint *array_of_statuses, MPI_Fint *ierror ) {
  #ifdef DEBUG
    print_calling_function();
  #endif
  int i;

  request_t * newHtab = NULL;

  for(i = 0; i < *count; i++)
  {
    newHtab = get_request(array_of_requests+i);

    //Request can be for rcv or send, if the request is not registered in the
    //htable it means this request was for a send.
    if(newHtab == NULL) continue;

    read_in_file(newHtab->buffer, newHtab->bufsize, newHtab->datatype);
    del_request(&requestHtab, hash_pointer(array_of_requests+i, 0), newHtab);
  }

  *ierror = MPI_SUCCESS;
}

int MPI_Wait( MPI_Request * request, MPI_Status * status ) {
  #ifdef DEBUG
    print_calling_function();
  #endif
  request_t * newHtab = NULL;

  //If newHtab == NULL it means that this request was for an isend and
  //therefore it was not registered in the htable
  newHtab = get_request(request);

  if(newHtab != NULL) {
    read_in_file(newHtab->buffer, newHtab->bufsize, newHtab->datatype);
    del_request(&requestHtab, hash_pointer(request, 0), newHtab);
  }

  return MPI_SUCCESS;
}

void mpi_wait_( MPI_Fint * request, MPI_Fint * status, int * ierror ) {
  #ifdef DEBUG
    print_calling_function();
  #endif
  request_t * newHtab = NULL;

  //If newHtab == NULL it means that this request was for an isend and
  //therefore it was not registered in the htable
  newHtab = get_request(request);

  if(newHtab != NULL) {
    read_in_file(newHtab->buffer, newHtab->bufsize, newHtab->datatype);
    del_request(&requestHtab, hash_pointer(request, 0), newHtab);
  }

  *ierror = MPI_SUCCESS;
}

//~int MPI_Bsend( const void * buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm ) {return MPI_SUCCESS;}
//~void mpi_bsend_( const void * buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Ibcast( void * buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPI_Request * request ) {return MPI_SUCCESS;}

//~int MPI_Bsend_init( const void * buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request * request ) {return MPI_SUCCESS;}
//~void mpi_bsend_init_( const void * buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request * request, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Buffer_attach( void * buffer, int size ) {return MPI_SUCCESS;}
//~void mpi_buffer_attach_( void * buffer, int size, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Buffer_detach( void * buffer, int * size ) {return MPI_SUCCESS;}
//~void mpi_buffer_detach_( void * buffer, int * size, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Cancel( MPI_Request * request ) {return MPI_SUCCESS;}
//~void mpi_cancel_( MPI_Request * request, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Cart_coords( MPI_Comm comm, int rank, int maxdims, int coords[] ) {return MPI_SUCCESS;}
//~void mpi_cart_coords_( MPI_Comm comm, int rank, int maxdims, int coords[], int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Cart_create( MPI_Comm old_comm, int ndims, const int dims[], const int periods[], int reorder, MPI_Comm * comm_cart ) {return MPI_SUCCESS;}
//~void mpi_cart_create_( MPI_Comm old_comm, int ndims, const int dims[], const int periods[], int reorder, MPI_Comm * comm_cart, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Cart_get( MPI_Comm comm, int maxdims, int dims[], int periods[], int coords[] ) {return MPI_SUCCESS;}
//~void mpi_cart_get_( MPI_Comm comm, int maxdims, int dims[], int periods[], int coords[], int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Cart_map( MPI_Comm comm, int ndims, const int dims[], const int periods[], int * newrank ) {return MPI_SUCCESS;}
//~void mpi_cart_map_( MPI_Comm comm, int ndims, const int dims[], const int periods[], int * newrank, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Cart_rank( MPI_Comm comm, const int coords[], int * rank ) {return MPI_SUCCESS;}
//~void mpi_cart_rank_( MPI_Comm comm, const int coords[], int * rank, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Cart_shift( MPI_Comm comm, int direction, int disp, int * rank_source, int * rank_dest ) {return MPI_SUCCESS;}
//~void mpi_cart_shift_( MPI_Comm comm, int direction, int disp, int * rank_source, int * rank_dest, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Cart_sub( MPI_Comm comm, const int remain_dims[], MPI_Comm * new_comm ) {return MPI_SUCCESS;}
//~void mpi_cart_sub_( MPI_Comm comm, const int remain_dims[], MPI_Comm * new_comm, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Cartdim_get( MPI_Comm comm, int * ndims ) {return MPI_SUCCESS;}
//~void mpi_cartdim_get_( MPI_Comm comm, int * ndims, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Close_port( const char * port_name ) {return MPI_SUCCESS;}
//~void mpi_close_port_( const char * port_name, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Comm_accept( const char * port_name, MPI_Info info, int root, MPI_Comm comm, MPI_Comm * newcomm ) {return MPI_SUCCESS;}
//~void mpi_comm_accept_( const char * port_name, MPI_Info info, int root, MPI_Comm comm, MPI_Comm * newcomm, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Comm_call_errhandler( MPI_Comm comm, int errorcode ) {return MPI_SUCCESS;}
//~void mpi_comm_call_errhandler_( MPI_Comm comm, int errorcode, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Comm_compare( MPI_Comm comm1, MPI_Comm comm2, int * result ) {return MPI_SUCCESS;}
//~void mpi_comm_compare_( MPI_Comm comm1, MPI_Comm comm2, int * result, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Comm_connect( const char * port_name, MPI_Info info, int root, MPI_Comm comm, MPI_Comm * newcomm ) {return MPI_SUCCESS;}
//~void mpi_comm_connect_( const char * port_name, MPI_Info info, int root, MPI_Comm comm, MPI_Comm * newcomm, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Comm_create_errhandler( MPI_Comm_errhandler_function * function, MPI_Errhandler * errhandler ) {return MPI_SUCCESS;}
//~void mpi_comm_create_errhandler_( MPI_Comm_errhandler_function * function, MPI_Errhandler * errhandler, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Comm_create_keyval( MPI_Comm_copy_attr_function * comm_copy_attr_fn, MPI_Comm_delete_attr_function * comm_delete_attr_fn, int * comm_keyval, void * extra_state ) {return MPI_SUCCESS;}
//~void mpi_comm_create_keyval_( MPI_Comm_copy_attr_function * comm_copy_attr_fn, MPI_Comm_delete_attr_function * comm_delete_attr_fn, int * comm_keyval, void * extra_state, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Comm_create_group( MPI_Comm comm, MPI_Group group, int tag, MPI_Comm * newcomm ) {return MPI_SUCCESS;}
//~void mpi_comm_create_group_( MPI_Comm comm, MPI_Group group, int tag, MPI_Comm * newcomm, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Comm_create( MPI_Comm comm, MPI_Group group, MPI_Comm * newcomm ) {return MPI_SUCCESS;}
//~void mpi_comm_create_( MPI_Comm comm, MPI_Group group, MPI_Comm * newcomm, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Comm_delete_attr( MPI_Comm comm, int comm_keyval ) {return MPI_SUCCESS;}
//~void mpi_comm_delete_attr_( MPI_Comm comm, int comm_keyval, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Comm_disconnect( MPI_Comm * comm ) {return MPI_SUCCESS;}
//~void mpi_comm_disconnect_( MPI_Comm * comm, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Comm_dup( MPI_Comm comm, MPI_Comm * newcomm ) {return MPI_SUCCESS;}
//~void mpi_comm_dup_( MPI_Comm comm, MPI_Comm * newcomm, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Comm_idup( MPI_Comm comm, MPI_Comm * newcomm, MPI_Request * request ) {return MPI_SUCCESS;}
//~void mpi_comm_idup_( MPI_Comm comm, MPI_Comm * newcomm, MPI_Request * request, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Comm_dup_with_info( MPI_Comm comm, MPI_Info info, MPI_Comm * newcomm ) {return MPI_SUCCESS;}
//~void mpi_comm_dup_with_info_( MPI_Comm comm, MPI_Info info, MPI_Comm * newcomm, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Comm_free_keyval( int * comm_keyval ) {return MPI_SUCCESS;}
//~void mpi_comm_free_keyval_( int * comm_keyval, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Comm_free( MPI_Comm * comm ) {return MPI_SUCCESS;}
//~void mpi_comm_free_( MPI_Comm * comm, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Comm_get_attr( MPI_Comm comm, int comm_keyval, void * attribute_val, int * flag ) {return MPI_SUCCESS;}
//~void mpi_comm_get_attr_( MPI_Comm comm, int comm_keyval, void * attribute_val, int * flag, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Dist_graph_create( MPI_Comm comm_old, int n, const int nodes[], const int degrees[], const int targets[], const int weights[], MPI_Info info, int reorder, MPI_Comm * newcomm ) {return MPI_SUCCESS;}
//~void mpi_dist_graph_create_( MPI_Comm comm_old, int n, const int nodes[], const int degrees[], const int targets[], const int weights[], MPI_Info info, int reorder, MPI_Comm * newcomm, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Dist_graph_create_adjacent( MPI_Comm comm_old, int indegree, const int sources[], const int sourceweights[], int outdegree, const int destinations[], const int destweights[], MPI_Info info, int reorder, MPI_Comm * comm_dist_graph ) {return MPI_SUCCESS;}
//~void mpi_dist_graph_create_adjacent_( MPI_Comm comm_old, int indegree, const int sources[], const int sourceweights[], int outdegree, const int destinations[], const int destweights[], MPI_Info info, int reorder, MPI_Comm * comm_dist_graph, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Dist_graph_neighbors( MPI_Comm comm, int maxindegree, int sources[], int sourceweights[], int maxoutdegree, int destinations[], int destweights[] ) {return MPI_SUCCESS;}
//~void mpi_dist_graph_neighbors_( MPI_Comm comm, int maxindegree, int sources[], int sourceweights[], int maxoutdegree, int destinations[], int destweights[], int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Dist_graph_neighbors_count( MPI_Comm comm, int * inneighbors, int * outneighbors, int * weighted ) {return MPI_SUCCESS;}
//~void mpi_dist_graph_neighbors_count_( MPI_Comm comm, int * inneighbors, int * outneighbors, int * weighted, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Comm_get_errhandler( MPI_Comm comm, MPI_Errhandler * erhandler ) {return MPI_SUCCESS;}
//~void mpi_comm_get_errhandler_( MPI_Comm comm, MPI_Errhandler * erhandler, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Comm_get_info( MPI_Comm comm, MPI_Info * info_used ) {return MPI_SUCCESS;}
//~void mpi_comm_get_info_( MPI_Comm comm, MPI_Info * info_used, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Comm_get_name( MPI_Comm comm, char * comm_name, int * resultlen ) {return MPI_SUCCESS;}
//~void mpi_comm_get_name_( MPI_Comm comm, char * comm_name, int * resultlen, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Comm_get_parent( MPI_Comm * parent ) {return MPI_SUCCESS;}
//~void mpi_comm_get_parent_( MPI_Comm * parent, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Comm_group( MPI_Comm comm, MPI_Group * group ) {return MPI_SUCCESS;}
//~void mpi_comm_group_( MPI_Comm comm, MPI_Group * group, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Comm_join( int fd, MPI_Comm * intercomm ) {return MPI_SUCCESS;}
//~void mpi_comm_join_( int fd, MPI_Comm * intercomm, int * ierror ) {*ierror = MPI_SUCCESS;}

int MPI_Comm_rank( MPI_Comm comm, int * rank ) {*rank = MPIre_rank; return MPI_SUCCESS;}
void mpi_comm_rank_( MPI_Comm comm, int * rank, int * ierror ) {*rank = MPIre_rank; *ierror = MPI_SUCCESS;}

//~int MPI_Comm_remote_group( MPI_Comm comm, MPI_Group * group ) {return MPI_SUCCESS;}
//~void mpi_comm_remote_group_( MPI_Comm comm, MPI_Group * group, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Comm_remote_size( MPI_Comm comm, int * size ) {return MPI_SUCCESS;}
//~void mpi_comm_remote_size_( MPI_Comm comm, int * size, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Comm_set_attr( MPI_Comm comm, int comm_keyval, void * attribute_val ) {return MPI_SUCCESS;}
//~void mpi_comm_set_attr_( MPI_Comm comm, int comm_keyval, void * attribute_val, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Comm_set_errhandler( MPI_Comm comm, MPI_Errhandler errhandler ) {return MPI_SUCCESS;}
//~void mpi_comm_set_errhandler_( MPI_Comm comm, MPI_Errhandler errhandler, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Comm_set_info( MPI_Comm comm, MPI_Info info ) {return MPI_SUCCESS;}
//~void mpi_comm_set_info_( MPI_Comm comm, MPI_Info info, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Comm_set_name( MPI_Comm comm, const char * comm_name ) {return MPI_SUCCESS;}
//~void mpi_comm_set_name_( MPI_Comm comm, const char * comm_name, int * ierror ) {*ierror = MPI_SUCCESS;}

int MPI_Comm_size( MPI_Comm comm, int * size ) {
  #ifdef DEBUG
    print_calling_function();
  #endif
  *size=MPIre_size;
  return MPI_SUCCESS;
}
void mpi_comm_size_( MPI_Comm comm, int * size, int * ierror ) {
  #ifdef DEBUG
    print_calling_function();
  #endif
  *size=MPIre_size;
  *ierror = MPI_SUCCESS;
}

//~int MPI_Comm_spawn( const char * command, char * argv[], int maxprocs, MPI_Info info, int root, MPI_Comm comm, MPI_Comm * intercomm, int array_of_errcodes[] ) {return MPI_SUCCESS;}
//~void mpi_comm_spawn_( const char * command, char * argv[], int maxprocs, MPI_Info info, int root, MPI_Comm comm, MPI_Comm * intercomm, int array_of_errcodes[], int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Comm_spawn_multiple( int count, char * array_of_commands[], char ** array_of_argv[], const int array_of_maxprocs[], const MPI_Info array_of_info[], int root, MPI_Comm comm, MPI_Comm * intercomm, int array_of_errcodes[] ) {return MPI_SUCCESS;}
//~void mpi_comm_spawn_multiple_( int count, char * array_of_commands[], char ** array_of_argv[], const int array_of_maxprocs[], const MPI_Info array_of_info[], int root, MPI_Comm comm, MPI_Comm * intercomm, int array_of_errcodes[], int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Comm_split( MPI_Comm comm, int color, int key, MPI_Comm * newcomm ) {return MPI_SUCCESS;}
//~void mpi_comm_split_( MPI_Comm comm, int color, int key, MPI_Comm * newcomm, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Comm_split_type( MPI_Comm comm, int split_type, int key, MPI_Info info, MPI_Comm * newcomm ) {return MPI_SUCCESS;}
//~void mpi_comm_split_type_( MPI_Comm comm, int split_type, int key, MPI_Info info, MPI_Comm * newcomm, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Comm_test_inter( MPI_Comm comm, int * flag ) {return MPI_SUCCESS;}
//~void mpi_comm_test_inter_( MPI_Comm comm, int * flag, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Compare_and_swap( void * origin_addr, void * compare_addr, void * result_addr, MPI_Datatype datatype, int target_rank, MPI_Aint target_disp, MPI_Win win ) {return MPI_SUCCESS;}
//~void mpi_compare_and_swap_( void * origin_addr, void * compare_addr, void * result_addr, MPI_Datatype datatype, int target_rank, MPI_Aint target_disp, MPI_Win win, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Dims_create( int nnodes, int ndims, int dims[] ) {return MPI_SUCCESS;}
//~void mpi_dims_create_( int nnodes, int ndims, int dims[], int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Errhandler_create( MPI_Handler_function * function, MPI_Errhandler * errhandler ) {return MPI_SUCCESS;}
//~void mpi_errhandler_create_( MPI_Handler_function * function, MPI_Errhandler * errhandler, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Errhandler_free( MPI_Errhandler * errhandler ) {return MPI_SUCCESS;}
//~void mpi_errhandler_free_( MPI_Errhandler * errhandler, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Errhandler_get( MPI_Comm comm, MPI_Errhandler * errhandler ) {return MPI_SUCCESS;}
//~void mpi_errhandler_get_( MPI_Comm comm, MPI_Errhandler * errhandler, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Errhandler_set( MPI_Comm comm, MPI_Errhandler errhandler ) {return MPI_SUCCESS;}
//~void mpi_errhandler_set_( MPI_Comm comm, MPI_Errhandler errhandler, int * ierror ) {*ierror = MPI_SUCCESS;}

 //~int MPI_Error_class( int errorcode, int * errorclass ) {return MPI_SUCCESS;}
 //~void mpi_error_class_( int errorcode, int * errorclass, int * ierror ) {*ierror = MPI_SUCCESS;}

 //~int MPI_Error_string( int errorcode, char * string, int * resultlen ) {return MPI_SUCCESS;}
 //~void mpi_error_string_( int errorcode, char * string, int * resultlen, int * ierror ) {*ierror = MPI_SUCCESS;}

 /*int MPI_Exscan( const void * sendbuf, void * recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm ) {return MPI_SUCCESS;}
 void mpi_exscan_( const void * sendbuf, void * recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, int * ierror ) {*ierror = MPI_SUCCESS;}*/

//~int MPI_Fetch_and_op( void * origin_addr, void * result_addr, MPI_Datatype datatype, int target_rank, MPI_Aint target_disp, MPI_Op op, MPI_Win win ) {return MPI_SUCCESS;}
//~void mpi_fetch_and_op_( void * origin_addr, void * result_addr, MPI_Datatype datatype, int target_rank, MPI_Aint target_disp, MPI_Op op, MPI_Win win, int * ierror ) {*ierror = MPI_SUCCESS;}

 /*int MPI_Iexscan( const void * sendbuf, void * recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request * request ) {return MPI_SUCCESS;}
 void mpi_iexscan_( const void * sendbuf, void * recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request * request, int * ierror ) {*ierror = MPI_SUCCESS;}*/

//~int MPI_File_call_errhandler( MPI_File fh, int errorcode ) {return MPI_SUCCESS;}
//~void mpi_file_call_errhandler_( MPI_File fh, int errorcode, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_File_create_errhandler( MPI_File_errhandler_function * function, MPI_Errhandler * errhandler ) {return MPI_SUCCESS;}
//~void mpi_file_create_errhandler_( MPI_File_errhandler_function * function, MPI_Errhandler * errhandler, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_File_set_errhandler( MPI_File file, MPI_Errhandler errhandler ) {return MPI_SUCCESS;}
//~void mpi_file_set_errhandler_( MPI_File file, MPI_Errhandler errhandler, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_File_get_errhandler( MPI_File file, MPI_Errhandler * errhandler ) {return MPI_SUCCESS;}
//~void mpi_file_get_errhandler_( MPI_File file, MPI_Errhandler * errhandler, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_File_open( MPI_Comm comm, const char * filename, int amode, MPI_Info info, MPI_File * fh ) {return MPI_SUCCESS;}
//~void mpi_file_open_( MPI_Comm comm, const char * filename, int amode, MPI_Info info, MPI_File * fh, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_File_close( MPI_File * fh ) {return MPI_SUCCESS;}
//~void mpi_file_close_( MPI_File * fh, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_File_delete( const char * filename, MPI_Info info ) {return MPI_SUCCESS;}
//~void mpi_file_delete_( const char * filename, MPI_Info info, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_File_set_size( MPI_File fh, MPI_Offset size ) {return MPI_SUCCESS;}
//~void mpi_file_set_size_( MPI_File fh, MPI_Offset size, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_File_preallocate( MPI_File fh, MPI_Offset size ) {return MPI_SUCCESS;}
//~void mpi_file_preallocate_( MPI_File fh, MPI_Offset size, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_File_get_size( MPI_File fh, MPI_Offset * size ) {return MPI_SUCCESS;}
//~void mpi_file_get_size_( MPI_File fh, MPI_Offset * size, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_File_get_group( MPI_File fh, MPI_Group * group ) {return MPI_SUCCESS;}
//~void mpi_file_get_group_( MPI_File fh, MPI_Group * group, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_File_get_amode( MPI_File fh, int * amode ) {return MPI_SUCCESS;}
//~void mpi_file_get_amode_( MPI_File fh, int * amode, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_File_set_info( MPI_File fh, MPI_Info info ) {return MPI_SUCCESS;}
//~void mpi_file_set_info_( MPI_File fh, MPI_Info info, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_File_get_info( MPI_File fh, MPI_Info * info_used ) {return MPI_SUCCESS;}
//~void mpi_file_get_info_( MPI_File fh, MPI_Info * info_used, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_File_set_view( MPI_File fh, MPI_Offset disp, MPI_Datatype etype, MPI_Datatype filetype, const char * datarep, MPI_Info info ) {return MPI_SUCCESS;}
//~void mpi_file_set_view_( MPI_File fh, MPI_Offset disp, MPI_Datatype etype, MPI_Datatype filetype, const char * datarep, MPI_Info info, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_File_get_view( MPI_File fh, MPI_Offset * disp, MPI_Datatype * etype, MPI_Datatype * filetype, char * datarep ) {return MPI_SUCCESS;}
//~void mpi_file_get_view_( MPI_File fh, MPI_Offset * disp, MPI_Datatype * etype, MPI_Datatype * filetype, char * datarep, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_File_read_at( MPI_File fh, MPI_Offset offset, void * buf, int count, MPI_Datatype datatype, MPI_Status * status ) {return MPI_SUCCESS;}
//~void mpi_file_read_at_( MPI_File fh, MPI_Offset offset, void * buf, int count, MPI_Datatype datatype, MPI_Status * status, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_File_read_at_all( MPI_File fh, MPI_Offset offset, void * buf, int count, MPI_Datatype datatype, MPI_Status * status ) {return MPI_SUCCESS;}
//~void mpi_file_read_at_all_( MPI_File fh, MPI_Offset offset, void * buf, int count, MPI_Datatype datatype, MPI_Status * status, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_File_write_at( MPI_File fh, MPI_Offset offset, const void * buf, int count, MPI_Datatype datatype, MPI_Status * status ) {return MPI_SUCCESS;}
//~void mpi_file_write_at_( MPI_File fh, MPI_Offset offset, const void * buf, int count, MPI_Datatype datatype, MPI_Status * status, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_File_write_at_all( MPI_File fh, MPI_Offset offset, const void * buf, int count, MPI_Datatype datatype, MPI_Status * status ) {return MPI_SUCCESS;}
//~void mpi_file_write_at_all_( MPI_File fh, MPI_Offset offset, const void * buf, int count, MPI_Datatype datatype, MPI_Status * status, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_File_iread_at( MPI_File fh, MPI_Offset offset, void * buf, int count, MPI_Datatype datatype, MPI_Request * request ) {return MPI_SUCCESS;}
//~void mpi_file_iread_at_( MPI_File fh, MPI_Offset offset, void * buf, int count, MPI_Datatype datatype, MPI_Request * request, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_File_iwrite_at( MPI_File fh, MPI_Offset offset, const void * buf, int count, MPI_Datatype datatype, MPI_Request * request ) {return MPI_SUCCESS;}
//~void mpi_file_iwrite_at_( MPI_File fh, MPI_Offset offset, const void * buf, int count, MPI_Datatype datatype, MPI_Request * request, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_File_read( MPI_File fh, void * buf, int count, MPI_Datatype datatype, MPI_Status * status ) {return MPI_SUCCESS;}
//~void mpi_file_read_( MPI_File fh, void * buf, int count, MPI_Datatype datatype, MPI_Status * status, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_File_read_all( MPI_File fh, void * buf, int count, MPI_Datatype datatype, MPI_Status * status ) {return MPI_SUCCESS;}
//~void mpi_file_read_all_( MPI_File fh, void * buf, int count, MPI_Datatype datatype, MPI_Status * status, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_File_write( MPI_File fh, const void * buf, int count, MPI_Datatype datatype, MPI_Status * status ) {return MPI_SUCCESS;}
//~void mpi_file_write_( MPI_File fh, const void * buf, int count, MPI_Datatype datatype, MPI_Status * status, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_File_write_all( MPI_File fh, const void * buf, int count, MPI_Datatype datatype, MPI_Status * status ) {return MPI_SUCCESS;}
//~void mpi_file_write_all_( MPI_File fh, const void * buf, int count, MPI_Datatype datatype, MPI_Status * status, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_File_iread( MPI_File fh, void * buf, int count, MPI_Datatype datatype, MPI_Request * request ) {return MPI_SUCCESS;}
//~void mpi_file_iread_( MPI_File fh, void * buf, int count, MPI_Datatype datatype, MPI_Request * request, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_File_iwrite( MPI_File fh, const void * buf, int count, MPI_Datatype datatype, MPI_Request * request ) {return MPI_SUCCESS;}
//~void mpi_file_iwrite_( MPI_File fh, const void * buf, int count, MPI_Datatype datatype, MPI_Request * request, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_File_seek( MPI_File fh, MPI_Offset offset, int whence ) {return MPI_SUCCESS;}
//~void mpi_file_seek_( MPI_File fh, MPI_Offset offset, int whence, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_File_get_position( MPI_File fh, MPI_Offset * offset ) {return MPI_SUCCESS;}
//~void mpi_file_get_position_( MPI_File fh, MPI_Offset * offset, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_File_get_byte_offset( MPI_File fh, MPI_Offset offset, MPI_Offset * disp ) {return MPI_SUCCESS;}
//~void mpi_file_get_byte_offset_( MPI_File fh, MPI_Offset offset, MPI_Offset * disp, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_File_read_shared( MPI_File fh, void * buf, int count, MPI_Datatype datatype, MPI_Status * status ) {return MPI_SUCCESS;}
//~void mpi_file_read_shared_( MPI_File fh, void * buf, int count, MPI_Datatype datatype, MPI_Status * status, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_File_write_shared( MPI_File fh, const void * buf, int count, MPI_Datatype datatype, MPI_Status * status ) {return MPI_SUCCESS;}
//~void mpi_file_write_shared_( MPI_File fh, const void * buf, int count, MPI_Datatype datatype, MPI_Status * status, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_File_iread_shared( MPI_File fh, void * buf, int count, MPI_Datatype datatype, MPI_Request * request ) {return MPI_SUCCESS;}
//~void mpi_file_iread_shared_( MPI_File fh, void * buf, int count, MPI_Datatype datatype, MPI_Request * request, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_File_iwrite_shared( MPI_File fh, const void * buf, int count, MPI_Datatype datatype, MPI_Request * request ) {return MPI_SUCCESS;}
//~void mpi_file_iwrite_shared_( MPI_File fh, const void * buf, int count, MPI_Datatype datatype, MPI_Request * request, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_File_read_ordered( MPI_File fh, void * buf, int count, MPI_Datatype datatype, MPI_Status * status ) {return MPI_SUCCESS;}
//~void mpi_file_read_ordered_( MPI_File fh, void * buf, int count, MPI_Datatype datatype, MPI_Status * status, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_File_write_ordered( MPI_File fh, const void * buf, int count, MPI_Datatype datatype, MPI_Status * status ) {return MPI_SUCCESS;}
//~void mpi_file_write_ordered_( MPI_File fh, const void * buf, int count, MPI_Datatype datatype, MPI_Status * status, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_File_seek_shared( MPI_File fh, MPI_Offset offset, int whence ) {return MPI_SUCCESS;}
//~void mpi_file_seek_shared_( MPI_File fh, MPI_Offset offset, int whence, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_File_get_position_shared( MPI_File fh, MPI_Offset * offset ) {return MPI_SUCCESS;}
//~void mpi_file_get_position_shared_( MPI_File fh, MPI_Offset * offset, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_File_read_at_all_begin( MPI_File fh, MPI_Offset offset, void * buf, int count, MPI_Datatype datatype ) {return MPI_SUCCESS;}
//~void mpi_file_read_at_all_begin_( MPI_File fh, MPI_Offset offset, void * buf, int count, MPI_Datatype datatype, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_File_read_at_all_end( MPI_File fh, void * buf, MPI_Status * status ) {return MPI_SUCCESS;}
//~void mpi_file_read_at_all_end_( MPI_File fh, void * buf, MPI_Status * status, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_File_write_at_all_begin( MPI_File fh, MPI_Offset offset, const void * buf, int count, MPI_Datatype datatype ) {return MPI_SUCCESS;}
//~void mpi_file_write_at_all_begin_( MPI_File fh, MPI_Offset offset, const void * buf, int count, MPI_Datatype datatype, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_File_write_at_all_end( MPI_File fh, const void * buf, MPI_Status * status ) {return MPI_SUCCESS;}
//~void mpi_file_write_at_all_end_( MPI_File fh, const void * buf, MPI_Status * status, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_File_read_all_begin( MPI_File fh, void * buf, int count, MPI_Datatype datatype ) {return MPI_SUCCESS;}
//~void mpi_file_read_all_begin_( MPI_File fh, void * buf, int count, MPI_Datatype datatype, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_File_read_all_end( MPI_File fh, void * buf, MPI_Status * status ) {return MPI_SUCCESS;}
//~void mpi_file_read_all_end_( MPI_File fh, void * buf, MPI_Status * status, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_File_write_all_begin( MPI_File fh, const void * buf, int count, MPI_Datatype datatype ) {return MPI_SUCCESS;}
//~void mpi_file_write_all_begin_( MPI_File fh, const void * buf, int count, MPI_Datatype datatype, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_File_write_all_end( MPI_File fh, const void * buf, MPI_Status * status ) {return MPI_SUCCESS;}
//~void mpi_file_write_all_end_( MPI_File fh, const void * buf, MPI_Status * status, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_File_read_ordered_begin( MPI_File fh, void * buf, int count, MPI_Datatype datatype ) {return MPI_SUCCESS;}
//~void mpi_file_read_ordered_begin_( MPI_File fh, void * buf, int count, MPI_Datatype datatype, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_File_read_ordered_end( MPI_File fh, void * buf, MPI_Status * status ) {return MPI_SUCCESS;}
//~void mpi_file_read_ordered_end_( MPI_File fh, void * buf, MPI_Status * status, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_File_write_ordered_begin( MPI_File fh, const void * buf, int count, MPI_Datatype datatype ) {return MPI_SUCCESS;}
//~void mpi_file_write_ordered_begin_( MPI_File fh, const void * buf, int count, MPI_Datatype datatype, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_File_write_ordered_end( MPI_File fh, const void * buf, MPI_Status * status ) {return MPI_SUCCESS;}
//~void mpi_file_write_ordered_end_( MPI_File fh, const void * buf, MPI_Status * status, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_File_get_type_extent( MPI_File fh, MPI_Datatype datatype, MPI_Aint * extent ) {return MPI_SUCCESS;}
//~void mpi_file_get_type_extent_( MPI_File fh, MPI_Datatype datatype, MPI_Aint * extent, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_File_set_atomicity( MPI_File fh, int flag ) {return MPI_SUCCESS;}
//~void mpi_file_set_atomicity_( MPI_File fh, int flag, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_File_get_atomicity( MPI_File fh, int * flag ) {return MPI_SUCCESS;}
//~void mpi_file_get_atomicity_( MPI_File fh, int * flag, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_File_sync( MPI_File fh ) {return MPI_SUCCESS;}
//~void mpi_file_sync_( MPI_File fh, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Free_mem( void * base ) {return MPI_SUCCESS;}
//~void mpi_free_mem_( void * base, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Gather( const void * sendbuf, int sendcount, MPI_Datatype sendtype, void * recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm ) {return MPI_SUCCESS;}
//~int MPI_Igather( const void * sendbuf, int sendcount, MPI_Datatype sendtype, void * recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request * request ) {return MPI_SUCCESS;}
//~int MPI_Gatherv( const void * sendbuf, int sendcount, MPI_Datatype sendtype, void * recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, int root, MPI_Comm comm ) {return MPI_SUCCESS;}
//~int MPI_Igatherv( const void * sendbuf, int sendcount, MPI_Datatype sendtype, void * recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request * request ) {return MPI_SUCCESS;}

//~int MPI_Get_address( const void * location, MPI_Aint * address ) {return MPI_SUCCESS;}
//~void mpi_get_address_( const void * location, MPI_Aint * address, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Get_count( const MPI_Status * status, MPI_Datatype datatype, int * count ) {return MPI_SUCCESS;}
//~void mpi_get_count_( const MPI_Status * status, MPI_Datatype datatype, int * count, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Get_elements( const MPI_Status * status, MPI_Datatype datatype, int * count ) {return MPI_SUCCESS;}
//~void mpi_get_elements_( const MPI_Status * status, MPI_Datatype datatype, int * count, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Get_elements_x( const MPI_Status * status, MPI_Datatype datatype, MPI_Count * count ) {return MPI_SUCCESS;}
//~void mpi_get_elements_x_( const MPI_Status * status, MPI_Datatype datatype, MPI_Count * count, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Get( void * origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win ) {return MPI_SUCCESS;}
//~void mpi_get_( void * origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Get_accumulate( const void * origin_addr, int origin_count, MPI_Datatype origin_datatype, void * result_addr, int result_count, MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win ) {return MPI_SUCCESS;}
//~void mpi_get_accumulate_( const void * origin_addr, int origin_count, MPI_Datatype origin_datatype, void * result_addr, int result_count, MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Get_library_version( char * version, int * resultlen ) {return MPI_SUCCESS;}
//~void mpi_get_library_version_( char * version, int * resultlen, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Get_processor_name( char * name, int * resultlen ) {return MPI_SUCCESS;}
//~void mpi_get_processor_name_( char * name, int * resultlen, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Get_version( int * version, int * subversion ) {return MPI_SUCCESS;}
//~void mpi_get_version_( int * version, int * subversion, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Graph_create( MPI_Comm comm_old, int nnodes, const int index[], const int edges[], int reorder, MPI_Comm * comm_graph ) {return MPI_SUCCESS;}
//~void mpi_graph_create_( MPI_Comm comm_old, int nnodes, const int index[], const int edges[], int reorder, MPI_Comm * comm_graph, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Graph_get( MPI_Comm comm, int maxindex, int maxedges, int index[], int edges[] ) {return MPI_SUCCESS;}
//~void mpi_graph_get_( MPI_Comm comm, int maxindex, int maxedges, int index[], int edges[], int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Graph_map( MPI_Comm comm, int nnodes, const int index[], const int edges[], int * newrank ) {return MPI_SUCCESS;}
//~void mpi_graph_map_( MPI_Comm comm, int nnodes, const int index[], const int edges[], int * newrank, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Graph_neighbors_count( MPI_Comm comm, int rank, int * nneighbors ) {return MPI_SUCCESS;}
//~void mpi_graph_neighbors_count_( MPI_Comm comm, int rank, int * nneighbors, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Graph_neighbors( MPI_Comm comm, int rank, int maxneighbors, int neighbors[] ) {return MPI_SUCCESS;}
//~void mpi_graph_neighbors_( MPI_Comm comm, int rank, int maxneighbors, int neighbors[], int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Graphdims_get( MPI_Comm comm, int * nnodes, int * nedges ) {return MPI_SUCCESS;}
//~void mpi_graphdims_get_( MPI_Comm comm, int * nnodes, int * nedges, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Grequest_complete( MPI_Request request ) {return MPI_SUCCESS;}
//~void mpi_grequest_complete_( MPI_Request request, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Grequest_start( MPI_Grequest_query_function * query_fn, MPI_Grequest_free_function * free_fn, MPI_Grequest_cancel_function * cancel_fn, void * extra_state, MPI_Request * request ) {return MPI_SUCCESS;}
//~void mpi_grequest_start_( MPI_Grequest_query_function * query_fn, MPI_Grequest_free_function * free_fn, MPI_Grequest_cancel_function * cancel_fn, void * extra_state, MPI_Request * request, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Group_compare( MPI_Group group1, MPI_Group group2, int * result ) {return MPI_SUCCESS;}
//~void mpi_group_compare_( MPI_Group group1, MPI_Group group2, int * result, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Group_difference( MPI_Group group1, MPI_Group group2, MPI_Group * newgroup ) {return MPI_SUCCESS;}
//~void mpi_group_difference_( MPI_Group group1, MPI_Group group2, MPI_Group * newgroup, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Group_excl( MPI_Group group, int n, const int ranks[], MPI_Group * newgroup ) {return MPI_SUCCESS;}
//~void mpi_group_excl_( MPI_Group group, int n, const int ranks[], MPI_Group * newgroup, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Group_free( MPI_Group * group ) {return MPI_SUCCESS;}
//~void mpi_group_free_( MPI_Group * group, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Group_incl( MPI_Group group, int n, const int ranks[], MPI_Group * newgroup ) {return MPI_SUCCESS;}
//~void mpi_group_incl_( MPI_Group group, int n, const int ranks[], MPI_Group * newgroup, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Group_intersection( MPI_Group group1, MPI_Group group2, MPI_Group * newgroup ) {return MPI_SUCCESS;}
//~void mpi_group_intersection_( MPI_Group group1, MPI_Group group2, MPI_Group * newgroup, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Group_range_excl( MPI_Group group, int n, int ranges[][3], MPI_Group * newgroup ) {return MPI_SUCCESS;}
//~void mpi_group_range_excl_( MPI_Group group, int n, int ranges[][3], MPI_Group * newgroup, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Group_range_incl( MPI_Group group, int n, int ranges[][3], MPI_Group * newgroup ) {return MPI_SUCCESS;}
//~void mpi_group_range_incl_( MPI_Group group, int n, int ranges[][3], MPI_Group * newgroup, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Group_rank( MPI_Group group, int * rank ) {return MPI_SUCCESS;}
//~void mpi_group_rank_( MPI_Group group, int * rank, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Group_size( MPI_Group group, int * size ) {return MPI_SUCCESS;}
//~void mpi_group_size_( MPI_Group group, int * size, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Group_translate_ranks( MPI_Group group1, int n, const int ranks1[], MPI_Group group2, int ranks2[] ) {return MPI_SUCCESS;}
//~void mpi_group_translate_ranks_( MPI_Group group1, int n, const int ranks1[], MPI_Group group2, int ranks2[], int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Group_union( MPI_Group group1, MPI_Group group2, MPI_Group * newgroup ) {return MPI_SUCCESS;}
//~void mpi_group_union_( MPI_Group group1, MPI_Group group2, MPI_Group * newgroup, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Ibsend( const void * buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request * request ) {return MPI_SUCCESS;}
//~void mpi_ibsend_( const void * buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request * request, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Improbe( int source, int tag, MPI_Comm comm, int * flag, MPI_Message * message, MPI_Status * status ) {return MPI_SUCCESS;}
//~void mpi_improbe_( int source, int tag, MPI_Comm comm, int * flag, MPI_Message * message, MPI_Status * status, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Imrecv( void * buf, int count, MPI_Datatype type, MPI_Message * message, MPI_Request * request ) {return MPI_SUCCESS;}
//~void mpi_imrecv_( void * buf, int count, MPI_Datatype type, MPI_Message * message, MPI_Request * request, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Info_create( MPI_Info * info ) {return MPI_SUCCESS;}
//~void mpi_info_create_( MPI_Info * info, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Info_delete( MPI_Info info, const char * key ) {return MPI_SUCCESS;}
//~void mpi_info_delete_( MPI_Info info, const char * key, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Info_dup( MPI_Info info, MPI_Info * newinfo ) {return MPI_SUCCESS;}
//~void mpi_info_dup_( MPI_Info info, MPI_Info * newinfo, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Info_free( MPI_Info * info ) {return MPI_SUCCESS;}
//~void mpi_info_free_( MPI_Info * info, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Info_get( MPI_Info info, const char * key, int valuelen, char * value, int * flag ) {return MPI_SUCCESS;}
//~void mpi_info_get_( MPI_Info info, const char * key, int valuelen, char * value, int * flag, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Info_get_nkeys( MPI_Info info, int * nkeys ) {return MPI_SUCCESS;}
//~void mpi_info_get_nkeys_( MPI_Info info, int * nkeys, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Info_get_nthkey( MPI_Info info, int n, char * key ) {return MPI_SUCCESS;}
//~void mpi_info_get_nthkey_( MPI_Info info, int n, char * key, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Info_get_valuelen( MPI_Info info, const char * key, int * valuelen, int * flag ) {return MPI_SUCCESS;}
//~void mpi_info_get_valuelen_( MPI_Info info, const char * key, int * valuelen, int * flag, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Info_set( MPI_Info info, const char * key, const char * value ) {return MPI_SUCCESS;}
//~void mpi_info_set_( MPI_Info info, const char * key, const char * value, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Init_thread( int * argc, char *** argv, int required, int * provided ) {return MPI_SUCCESS;}
//~void mpi_init_thread_( int * argc, char *** argv, int required, int * provided, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Intercomm_create( MPI_Comm local_comm, int local_leader, MPI_Comm bridge_comm, int remote_leader, int tag, MPI_Comm * newintercomm ) {return MPI_SUCCESS;}
//~void mpi_intercomm_create_( MPI_Comm local_comm, int local_leader, MPI_Comm bridge_comm, int remote_leader, int tag, MPI_Comm * newintercomm, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Intercomm_merge( MPI_Comm intercomm, int high, MPI_Comm * newintercomm ) {return MPI_SUCCESS;}
//~void mpi_intercomm_merge_( MPI_Comm intercomm, int high, MPI_Comm * newintercomm, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Iprobe( int source, int tag, MPI_Comm comm, int * flag, MPI_Status * status ) {return MPI_SUCCESS;}
//~void mpi_iprobe_( int source, int tag, MPI_Comm comm, int * flag, MPI_Status * status, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Irsend( const void * buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request * request ) {return MPI_SUCCESS;}
//~void mpi_irsend_( const void * buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request * request, int * ierror ) {*ierror = MPI_SUCCESS;}

int MPI_Isend( const void * buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request * request ) {return MPI_SUCCESS;}
void mpi_isend_( const void * buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request * request, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Issend( const void * buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request * request ) {return MPI_SUCCESS;}
//~void mpi_issend_( const void * buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request * request, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Is_thread_main( int * flag ) {return MPI_SUCCESS;}
//~void mpi_is_thread_main_( int * flag, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Keyval_create( MPI_Copy_function * copy_fn, MPI_Delete_function * delete_fn, int * keyval, void * extra_state ) {return MPI_SUCCESS;}
//~void mpi_keyval_create_( MPI_Copy_function * copy_fn, MPI_Delete_function * delete_fn, int * keyval, void * extra_state, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Keyval_free( int * keyval ) {return MPI_SUCCESS;}
//~void mpi_keyval_free_( int * keyval, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Lookup_name( const char * service_name, MPI_Info info, char * port_name ) {return MPI_SUCCESS;}
//~void mpi_lookup_name_( const char * service_name, MPI_Info info, char * port_name, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Mprobe( int source, int tag, MPI_Comm comm, MPI_Message * message, MPI_Status * status ) {return MPI_SUCCESS;}
//~void mpi_mprobe_( int source, int tag, MPI_Comm comm, MPI_Message * message, MPI_Status * status, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Mrecv( void * buf, int count, MPI_Datatype type, MPI_Message * message, MPI_Status * status ) {return MPI_SUCCESS;}
//~void mpi_mrecv_( void * buf, int count, MPI_Datatype type, MPI_Message * message, MPI_Status * status, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Neighbor_allgather( const void * sendbuf, int sendcount, MPI_Datatype sendtype, void * recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm ) {return MPI_SUCCESS;}
//~void mpi_neighbor_allgather_( const void * sendbuf, int sendcount, MPI_Datatype sendtype, void * recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Ineighbor_allgather( const void * sendbuf, int sendcount, MPI_Datatype sendtype, void * recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request * request ) {return MPI_SUCCESS;}
//~void mpi_ineighbor_allgather_( const void * sendbuf, int sendcount, MPI_Datatype sendtype, void * recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request * request, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Neighbor_allgatherv( const void * sendbuf, int sendcount, MPI_Datatype sendtype, void * recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, MPI_Comm comm ) {return MPI_SUCCESS;}
//~void mpi_neighbor_allgatherv_( const void * sendbuf, int sendcount, MPI_Datatype sendtype, void * recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, MPI_Comm comm, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Ineighbor_allgatherv( const void * sendbuf, int sendcount, MPI_Datatype sendtype, void * recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request * request ) {return MPI_SUCCESS;}
//~void mpi_ineighbor_allgatherv_( const void * sendbuf, int sendcount, MPI_Datatype sendtype, void * recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request * request, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Neighbor_alltoall( const void * sendbuf, int sendcount, MPI_Datatype sendtype, void * recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm ) {return MPI_SUCCESS;}
//~void mpi_neighbor_alltoall_( const void * sendbuf, int sendcount, MPI_Datatype sendtype, void * recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Ineighbor_alltoall( const void * sendbuf, int sendcount, MPI_Datatype sendtype, void * recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request * request ) {return MPI_SUCCESS;}
//~void mpi_ineighbor_alltoall_( const void * sendbuf, int sendcount, MPI_Datatype sendtype, void * recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request * request, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Neighbor_alltoallv( const void * sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype, void * recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm ) {return MPI_SUCCESS;}
//~void mpi_neighbor_alltoallv_( const void * sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype, void * recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Ineighbor_alltoallv( const void * sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype, void * recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request * request ) {return MPI_SUCCESS;}
//~void mpi_ineighbor_alltoallv_( const void * sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype, void * recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request * request, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Neighbor_alltoallw( const void * sendbuf, const int sendcounts[], const MPI_Aint sdispls[], const MPI_Datatype sendtypes[], void * recvbuf, const int recvcounts[], const MPI_Aint rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm ) {return MPI_SUCCESS;}
//~void mpi_neighbor_alltoallw_( const void * sendbuf, const int sendcounts[], const MPI_Aint sdispls[], const MPI_Datatype sendtypes[], void * recvbuf, const int recvcounts[], const MPI_Aint rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Ineighbor_alltoallw( const void * sendbuf, const int sendcounts[], const MPI_Aint sdispls[], const MPI_Datatype sendtypes[], void * recvbuf, const int recvcounts[], const MPI_Aint rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm, MPI_Request * request ) {return MPI_SUCCESS;}
//~void mpi_ineighbor_alltoallw_( const void * sendbuf, const int sendcounts[], const MPI_Aint sdispls[], const MPI_Datatype sendtypes[], void * recvbuf, const int recvcounts[], const MPI_Aint rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm, MPI_Request * request, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Op_commutative( MPI_Op op, int * commute ) {return MPI_SUCCESS;}
//~void mpi_op_commutative_( MPI_Op op, int * commute, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Op_create( MPI_User_function * function, int commute, MPI_Op * op ) {return MPI_SUCCESS;}
//~void mpi_op_create_( MPI_User_function * function, int commute, MPI_Op * op, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Open_port( MPI_Info info, char * port_name ) {return MPI_SUCCESS;}
//~void mpi_open_port_( MPI_Info info, char * port_name, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Op_free( MPI_Op * op ) {return MPI_SUCCESS;}
//~void mpi_op_free_( MPI_Op * op, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Pack_external( const char datarep[], const void * inbuf, int incount, MPI_Datatype datatype, void * outbuf, MPI_Aint outsize, MPI_Aint * position ) {return MPI_SUCCESS;}
//~void mpi_pack_external_( const char datarep[], const void * inbuf, int incount, MPI_Datatype datatype, void * outbuf, MPI_Aint outsize, MPI_Aint * position, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Pack_external_size( const char datarep[], int incount, MPI_Datatype datatype, MPI_Aint * size ) {return MPI_SUCCESS;}
//~void mpi_pack_external_size_( const char datarep[], int incount, MPI_Datatype datatype, MPI_Aint * size, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Pack( const void * inbuf, int incount, MPI_Datatype datatype, void * outbuf, int outsize, int * position, MPI_Comm comm ) {return MPI_SUCCESS;}
//~void mpi_pack_( const void * inbuf, int incount, MPI_Datatype datatype, void * outbuf, int outsize, int * position, MPI_Comm comm, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Pack_size( int incount, MPI_Datatype datatype, MPI_Comm comm, int * size ) {return MPI_SUCCESS;}
//~void mpi_pack_size_( int incount, MPI_Datatype datatype, MPI_Comm comm, int * size, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Probe( int source, int tag, MPI_Comm comm, MPI_Status * status ) {return MPI_SUCCESS;}
//~void mpi_probe_( int source, int tag, MPI_Comm comm, MPI_Status * status, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Publish_name( const char * service_name, MPI_Info info, const char * port_name ) {return MPI_SUCCESS;}
//~void mpi_publish_name_( const char * service_name, MPI_Info info, const char * port_name, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Put( const void * origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win ) {return MPI_SUCCESS;}
//~void mpi_put_( const void * origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Query_thread( int * provided ) {return MPI_SUCCESS;}
//~void mpi_query_thread_( int * provided, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Raccumulate( void * origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win, MPI_Request * request ) {return MPI_SUCCESS;}
//~void mpi_raccumulate_( void * origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win, MPI_Request * request, int * ierror ) {*ierror = MPI_SUCCESS;}

int MPI_Reduce( const void * sendbuf, void * recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm ) {
  #ifdef DEBUG
    print_calling_function();
  #endif
  if(MPIre_rank == root)
    read_in_file(recvbuf, count, datatype);

  return MPI_SUCCESS;
}

void mpi_reduce( MPI_Fint * sendbuf, MPI_Fint * recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *root, MPI_Fint *comm, MPI_Fint * ierror ) {
  MPI_Comm c_comm = MPI_Comm_f2c(*comm);
  MPI_Datatype c_datatype = MPI_Type_f2c(*datatype);
  MPI_Op c_op = MPI_Op_f2c(*op);
  *ierror = MPI_Reduce(sendbuf, recvbuf, *count, c_datatype, c_op, *root, c_comm);
}

//~int MPI_Reduce_local( const void * inbuf, void * inoutbuf, int count, MPI_Datatype datatype, MPI_Op op ) {return MPI_SUCCESS;}
//~void mpi_reduce_local_( const void * inbuf, void * inoutbuf, int count, MPI_Datatype datatype, MPI_Op op, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Ireduce( const void * sendbuf, void * recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPI_Request * request ) {return MPI_SUCCESS;}
//~int MPI_Reduce_scatter( const void * sendbuf, void * recvbuf, const int recvcounts[], MPI_Datatype datatype, MPI_Op op, MPI_Comm comm ) {return MPI_SUCCESS;}
//~int MPI_Ireduce_scatter( const void * sendbuf, void * recvbuf, const int recvcounts[], MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request * request ) {return MPI_SUCCESS;}
//~int MPI_Reduce_scatter_block( const void * sendbuf, void * recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm ) {return MPI_SUCCESS;}
//~int MPI_Ireduce_scatter_block( const void * sendbuf, void * recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request * request ) {return MPI_SUCCESS;}

//~int MPI_Register_datarep( const char * datarep, MPI_Datarep_conversion_function * read_conversion_fn, MPI_Datarep_conversion_function * write_conversion_fn, MPI_Datarep_extent_function * dtype_file_extent_fn, void * extra_state ) {return MPI_SUCCESS;}
//~void mpi_register_datarep_( const char * datarep, MPI_Datarep_conversion_function * read_conversion_fn, MPI_Datarep_conversion_function * write_conversion_fn, MPI_Datarep_extent_function * dtype_file_extent_fn, void * extra_state, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Request_free( MPI_Request * request ) {return MPI_SUCCESS;}
//~void mpi_request_free_( MPI_Request * request, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Request_get_status( MPI_Request request, int * flag, MPI_Status * status ) {return MPI_SUCCESS;}
//~void mpi_request_get_status_( MPI_Request request, int * flag, MPI_Status * status, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Rget( void * origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win, MPI_Request * request ) {return MPI_SUCCESS;}
//~void mpi_rget_( void * origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win, MPI_Request * request, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Rget_accumulate( const void * origin_addr, int origin_count, MPI_Datatype origin_datatype, void * result_addr, int result_count, MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win, MPI_Request * request ) {return MPI_SUCCESS;}
//~void mpi_rget_accumulate_( const void * origin_addr, int origin_count, MPI_Datatype origin_datatype, void * result_addr, int result_count, MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win, MPI_Request * request, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Rput( const void * origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_cout, MPI_Datatype target_datatype, MPI_Win win, MPI_Request * request ) {return MPI_SUCCESS;}
//~void mpi_rput_( const void * origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_cout, MPI_Datatype target_datatype, MPI_Win win, MPI_Request * request, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Rsend( const void * ibuf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm ) {return MPI_SUCCESS;}
//~void mpi_rsend_( const void * ibuf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Rsend_init( const void * buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request * request ) {return MPI_SUCCESS;}
//~void mpi_rsend_init_( const void * buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request * request, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Scan( const void * sendbuf, void * recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm ) {return MPI_SUCCESS;}
//~int MPI_Iscan( const void * sendbuf, void * recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request * request ) {return MPI_SUCCESS;}
//~int MPI_Scatter( const void * sendbuf, int sendcount, MPI_Datatype sendtype, void * recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm ) {return MPI_SUCCESS;}
//~int MPI_Iscatter( const void * sendbuf, int sendcount, MPI_Datatype sendtype, void * recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request * request ) {return MPI_SUCCESS;}
//~int MPI_Scatterv( const void * sendbuf, const int sendcounts[], const int displs[], MPI_Datatype sendtype, void * recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm ) {return MPI_SUCCESS;}
//~int MPI_Iscatterv( const void * sendbuf, const int sendcounts[], const int displs[], MPI_Datatype sendtype, void * recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request * request ) {return MPI_SUCCESS;}

//~int MPI_Send_init( const void * buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request * request ) {return MPI_SUCCESS;}
//~void mpi_send_init_( const void * buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request * request, int * ierror ) {*ierror = MPI_SUCCESS;}

int MPI_Send( const void * buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm ) {return MPI_SUCCESS;}
void mpi_send_( const void * buf, MPI_Fint * count, MPI_Fint * datatype, MPI_Fint * dest, MPI_Fint * tag, MPI_Fint * comm, MPI_Fint * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Ssend_init( const void * buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request * request ) {return MPI_SUCCESS;}
//~void mpi_ssend_init_( const void * buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request * request, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Ssend( const void * buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm ) {return MPI_SUCCESS;}
//~void mpi_ssend_( const void * buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, int * ierror ) {*ierror = MPI_SUCCESS;}

int MPI_Start( MPI_Request * request ) {return MPI_SUCCESS;}
void mpi_start_( MPI_Request * request, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Recv_init( void * buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request * request ) {}
//~void mpi_recv_init_( void * buf, int * count, int * datatype, int * source, int * tag, int * comm, int * request , int * ierror ) {}

//~int MPI_Startall( int count, MPI_Request array_of_requests[] ) {return MPI_SUCCESS;}
//~void mpi_startall_( int count, MPI_Request array_of_requests[], int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Status_set_cancelled( MPI_Status * status, int flag ) {return MPI_SUCCESS;}
//~void mpi_status_set_cancelled_( MPI_Status * status, int flag, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Status_set_elements( MPI_Status * status, MPI_Datatype datatype, int count ) {return MPI_SUCCESS;}
//~void mpi_status_set_elements_( MPI_Status * status, MPI_Datatype datatype, int count, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Status_set_elements_x( MPI_Status * status, MPI_Datatype datatype, MPI_Count count ) {return MPI_SUCCESS;}
//~void mpi_status_set_elements_x_( MPI_Status * status, MPI_Datatype datatype, MPI_Count count, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Testall( int count, MPI_Request array_of_requests[], int * flag, MPI_Status array_of_statuses[] ) {return MPI_SUCCESS;}
//~void mpi_testall_( int count, MPI_Request array_of_requests[], int * flag, MPI_Status array_of_statuses[], int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Testany( int count, MPI_Request array_of_requests[], int * index, int * flag, MPI_Status * status ) {return MPI_SUCCESS;}
//~void mpi_testany_( int count, MPI_Request array_of_requests[], int * index, int * flag, MPI_Status * status, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Test( MPI_Request * request, int * flag, MPI_Status * status ) {return MPI_SUCCESS;}
//~void mpi_test_( MPI_Request * request, int * flag, MPI_Status * status, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Test_cancelled( const MPI_Status * status, int * flag ) {return MPI_SUCCESS;}
//~void mpi_test_cancelled_( const MPI_Status * status, int * flag, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Testsome( int incount, MPI_Request array_of_requests[], int * outcount, int array_of_indices[], MPI_Status array_of_statuses[] ) {return MPI_SUCCESS;}
//~void mpi_testsome_( int incount, MPI_Request array_of_requests[], int * outcount, int array_of_indices[], MPI_Status array_of_statuses[], int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Topo_test( MPI_Comm comm, int * status ) {return MPI_SUCCESS;}
//~void mpi_topo_test_( MPI_Comm comm, int * status, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Type_commit( MPI_Datatype * type ) {return MPI_SUCCESS;}
//~void mpi_type_commit_( MPI_Datatype * type, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Type_contiguous( int count, MPI_Datatype oldtype, MPI_Datatype * newtype ) {return MPI_SUCCESS;}
//~void mpi_type_contiguous_( int count, MPI_Datatype oldtype, MPI_Datatype * newtype, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Type_create_darray( int size, int rank, int ndims, const int gsize_array[], const int distrib_array[], const int darg_array[], const int psize_array[], int order, MPI_Datatype oldtype, MPI_Datatype * newtype ) {return MPI_SUCCESS;}
//~void mpi_type_create_darray_( int size, int rank, int ndims, const int gsize_array[], const int distrib_array[], const int darg_array[], const int psize_array[], int order, MPI_Datatype oldtype, MPI_Datatype * newtype, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Type_create_f90_complex( int p, int r, MPI_Datatype * newtype ) {return MPI_SUCCESS;}
//~void mpi_type_create_f90_complex_( int p, int r, MPI_Datatype * newtype, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Type_create_f90_integer( int r, MPI_Datatype * newtype ) {return MPI_SUCCESS;}
//~void mpi_type_create_f90_integer_( int r, MPI_Datatype * newtype, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Type_create_f90_real( int p, int r, MPI_Datatype * newtype ) {return MPI_SUCCESS;}
//~void mpi_type_create_f90_real_( int p, int r, MPI_Datatype * newtype, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Type_create_hindexed_block( int count, int blocklength, const MPI_Aint array_of_displacements[], MPI_Datatype oldtype, MPI_Datatype * newtype ) {return MPI_SUCCESS;}
//~void mpi_type_create_hindexed_block_( int count, int blocklength, const MPI_Aint array_of_displacements[], MPI_Datatype oldtype, MPI_Datatype * newtype, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Type_create_hindexed( int count, const int array_of_blocklengths[], const MPI_Aint array_of_displacements[], MPI_Datatype oldtype, MPI_Datatype * newtype ) {return MPI_SUCCESS;}
//~void mpi_type_create_hindexed_( int count, const int array_of_blocklengths[], const MPI_Aint array_of_displacements[], MPI_Datatype oldtype, MPI_Datatype * newtype, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Type_create_hvector( int count, int blocklength, MPI_Aint stride, MPI_Datatype oldtype, MPI_Datatype * newtype ) {return MPI_SUCCESS;}
//~void mpi_type_create_hvector_( int count, int blocklength, MPI_Aint stride, MPI_Datatype oldtype, MPI_Datatype * newtype, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Type_create_keyval( MPI_Type_copy_attr_function * type_copy_attr_fn, MPI_Type_delete_attr_function * type_delete_attr_fn, int * type_keyval, void * extra_state ) {return MPI_SUCCESS;}
//~void mpi_type_create_keyval_( MPI_Type_copy_attr_function * type_copy_attr_fn, MPI_Type_delete_attr_function * type_delete_attr_fn, int * type_keyval, void * extra_state, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Type_create_indexed_block( int count, int blocklength, const int array_of_displacements[], MPI_Datatype oldtype, MPI_Datatype * newtype ) {return MPI_SUCCESS;}
//~void mpi_type_create_indexed_block_( int count, int blocklength, const int array_of_displacements[], MPI_Datatype oldtype, MPI_Datatype * newtype, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Type_create_struct( int count, const int array_of_block_lengths[], const MPI_Aint array_of_displacements[], const MPI_Datatype array_of_types[], MPI_Datatype * newtype ) {return MPI_SUCCESS;}
//~void mpi_type_create_struct_( int count, const int array_of_block_lengths[], const MPI_Aint array_of_displacements[], const MPI_Datatype array_of_types[], MPI_Datatype * newtype, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Type_create_subarray( int ndims, const int size_array[], const int subsize_array[], const int start_array[], int order, MPI_Datatype oldtype, MPI_Datatype * newtype ) {return MPI_SUCCESS;}
//~void mpi_type_create_subarray_( int ndims, const int size_array[], const int subsize_array[], const int start_array[], int order, MPI_Datatype oldtype, MPI_Datatype * newtype, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Type_create_resized( MPI_Datatype oldtype, MPI_Aint lb, MPI_Aint extent, MPI_Datatype * newtype ) {return MPI_SUCCESS;}
//~void mpi_type_create_resized_( MPI_Datatype oldtype, MPI_Aint lb, MPI_Aint extent, MPI_Datatype * newtype, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Type_delete_attr( MPI_Datatype type, int type_keyval ) {return MPI_SUCCESS;}
//~void mpi_type_delete_attr_( MPI_Datatype type, int type_keyval, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Type_dup( MPI_Datatype type, MPI_Datatype * newtype ) {return MPI_SUCCESS;}
//~void mpi_type_dup_( MPI_Datatype type, MPI_Datatype * newtype, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Type_extent( MPI_Datatype type, MPI_Aint * extent ) {return MPI_SUCCESS;}
//~void mpi_type_extent_( MPI_Datatype type, MPI_Aint * extent, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Type_free( MPI_Datatype * type ) {return MPI_SUCCESS;}
//~void mpi_type_free_( MPI_Datatype * type, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Type_free_keyval( int * type_keyval ) {return MPI_SUCCESS;}
//~void mpi_type_free_keyval_( int * type_keyval, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Type_get_attr( MPI_Datatype type, int type_keyval, void * attribute_val, int * flag ) {return MPI_SUCCESS;}
//~void mpi_type_get_attr_( MPI_Datatype type, int type_keyval, void * attribute_val, int * flag, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Type_get_contents( MPI_Datatype mtype, int max_integers, int max_addresses, int max_datatypes, int array_of_integers[], MPI_Aint array_of_addresses[], MPI_Datatype array_of_datatypes[] ) {return MPI_SUCCESS;}
//~void mpi_type_get_contents_( MPI_Datatype mtype, int max_integers, int max_addresses, int max_datatypes, int array_of_integers[], MPI_Aint array_of_addresses[], MPI_Datatype array_of_datatypes[], int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Type_get_envelope( MPI_Datatype type, int * num_integers, int * num_addresses, int * num_datatypes, int * combiner ) {return MPI_SUCCESS;}
//~void mpi_type_get_envelope_( MPI_Datatype type, int * num_integers, int * num_addresses, int * num_datatypes, int * combiner, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Type_get_extent( MPI_Datatype type, MPI_Aint * lb, MPI_Aint * extent ) {return MPI_SUCCESS;}
//~void mpi_type_get_extent_( MPI_Datatype type, MPI_Aint * lb, MPI_Aint * extent, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Type_get_extent_x( MPI_Datatype type, MPI_Count * lb, MPI_Count * extent ) {return MPI_SUCCESS;}
//~void mpi_type_get_extent_x_( MPI_Datatype type, MPI_Count * lb, MPI_Count * extent, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Type_get_name( MPI_Datatype type, char * type_name, int * resultlen ) {return MPI_SUCCESS;}
//~void mpi_type_get_name_( MPI_Datatype type, char * type_name, int * resultlen, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Type_get_true_extent( MPI_Datatype datatype, MPI_Aint * true_lb, MPI_Aint * true_extent ) {return MPI_SUCCESS;}
//~void mpi_type_get_true_extent_( MPI_Datatype datatype, MPI_Aint * true_lb, MPI_Aint * true_extent, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Type_get_true_extent_x( MPI_Datatype datatype, MPI_Count * true_lb, MPI_Count * true_extent ) {return MPI_SUCCESS;}
//~void mpi_type_get_true_extent_x_( MPI_Datatype datatype, MPI_Count * true_lb, MPI_Count * true_extent, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Type_hindexed( int count, int array_of_blocklengths[], MPI_Aint array_of_displacements[], MPI_Datatype oldtype, MPI_Datatype * newtype ) {return MPI_SUCCESS;}
//~void mpi_type_hindexed_( int count, int array_of_blocklengths[], MPI_Aint array_of_displacements[], MPI_Datatype oldtype, MPI_Datatype * newtype, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Type_hvector( int count, int blocklength, MPI_Aint stride, MPI_Datatype oldtype, MPI_Datatype * newtype ) {return MPI_SUCCESS;}
//~void mpi_type_hvector_( int count, int blocklength, MPI_Aint stride, MPI_Datatype oldtype, MPI_Datatype * newtype, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Type_indexed( int count, const int array_of_blocklengths[], const int array_of_displacements[], MPI_Datatype oldtype, MPI_Datatype * newtype ) {return MPI_SUCCESS;}
//~void mpi_type_indexed_( int count, const int array_of_blocklengths[], const int array_of_displacements[], MPI_Datatype oldtype, MPI_Datatype * newtype, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Type_lb( MPI_Datatype type, MPI_Aint * lb ) {return MPI_SUCCESS;}
//~void mpi_type_lb_( MPI_Datatype type, MPI_Aint * lb, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Type_match_size( int typeclass, int size, MPI_Datatype * type ) {return MPI_SUCCESS;}
//~void mpi_type_match_size_( int typeclass, int size, MPI_Datatype * type, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Type_set_attr( MPI_Datatype type, int type_keyval, void * attr_val ) {return MPI_SUCCESS;}
//~void mpi_type_set_attr_( MPI_Datatype type, int type_keyval, void * attr_val, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Type_set_name( MPI_Datatype type, const char * type_name ) {return MPI_SUCCESS;}
//~void mpi_type_set_name_( MPI_Datatype type, const char * type_name, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Type_size( MPI_Datatype type, int * size ) {return MPI_SUCCESS;}
//~void mpi_type_size_( MPI_Datatype type, int * size, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Type_size_x( MPI_Datatype type, MPI_Count * size ) {return MPI_SUCCESS;}
//~void mpi_type_size_x_( MPI_Datatype type, MPI_Count * size, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Type_struct( int count, int array_of_blocklengths[], MPI_Aint array_of_displacements[], MPI_Datatype array_of_types[], MPI_Datatype * newtype ) {return MPI_SUCCESS;}
//~void mpi_type_struct_( int count, int array_of_blocklengths[], MPI_Aint array_of_displacements[], MPI_Datatype array_of_types[], MPI_Datatype * newtype, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Type_ub( MPI_Datatype mtype, MPI_Aint * ub ) {return MPI_SUCCESS;}
//~void mpi_type_ub_( MPI_Datatype mtype, MPI_Aint * ub, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Type_vector( int count, int blocklength, int stride, MPI_Datatype oldtype, MPI_Datatype * newtype ) {return MPI_SUCCESS;}
//~void mpi_type_vector_( int count, int blocklength, int stride, MPI_Datatype oldtype, MPI_Datatype * newtype, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Unpack( const void * inbuf, int insize, int * position, void * outbuf, int outcount, MPI_Datatype datatype, MPI_Comm comm ) {return MPI_SUCCESS;}
//~void mpi_unpack_( const void * inbuf, int insize, int * position, void * outbuf, int outcount, MPI_Datatype datatype, MPI_Comm comm, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Unpublish_name( const char * service_name, MPI_Info info, const char * port_name ) {return MPI_SUCCESS;}
//~void mpi_unpublish_name_( const char * service_name, MPI_Info info, const char * port_name, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Unpack_external( const char datarep[], const void * inbuf, MPI_Aint insize, MPI_Aint * position, void * outbuf, int outcount, MPI_Datatype datatype ) {return MPI_SUCCESS;}
//~void mpi_unpack_external_( const char datarep[], const void * inbuf, MPI_Aint insize, MPI_Aint * position, void * outbuf, int outcount, MPI_Datatype datatype, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Waitany( int count, MPI_Request array_of_requests[], int * index, MPI_Status * status ) {return MPI_SUCCESS;}
//~void mpi_waitany_( int count, MPI_Request array_of_requests[], int * index, MPI_Status * status, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Waitsome( int incount, MPI_Request array_of_requests[], int * outcount, int array_of_indices[], MPI_Status array_of_statuses[] ) {return MPI_SUCCESS;}
//~void mpi_waitsome_( int incount, MPI_Request array_of_requests[], int * outcount, int array_of_indices[], MPI_Status array_of_statuses[], int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Win_allocate( MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, void * baseptr, MPI_Win * win ) {return MPI_SUCCESS;}
//~void mpi_win_allocate_( MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, void * baseptr, MPI_Win * win, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Win_allocate_shared( MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, void * baseptr, MPI_Win * win ) {return MPI_SUCCESS;}
//~void mpi_win_allocate_shared_( MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, void * baseptr, MPI_Win * win, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Win_attach( MPI_Win win, void * base, MPI_Aint size ) {return MPI_SUCCESS;}
//~void mpi_win_attach_( MPI_Win win, void * base, MPI_Aint size, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Win_call_errhandler( MPI_Win win, int errorcode ) {return MPI_SUCCESS;}
//~void mpi_win_call_errhandler_( MPI_Win win, int errorcode, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Win_complete( MPI_Win win ) {return MPI_SUCCESS;}
//~void mpi_win_complete_( MPI_Win win, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Win_create( void * base, MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, MPI_Win * win ) {return MPI_SUCCESS;}
//~void mpi_win_create_( void * base, MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, MPI_Win * win, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Win_create_dynamic( MPI_Info info, MPI_Comm comm, MPI_Win * win ) {return MPI_SUCCESS;}
//~void mpi_win_create_dynamic_( MPI_Info info, MPI_Comm comm, MPI_Win * win, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Win_create_errhandler( MPI_Win_errhandler_function * function, MPI_Errhandler * errhandler ) {return MPI_SUCCESS;}
//~void mpi_win_create_errhandler_( MPI_Win_errhandler_function * function, MPI_Errhandler * errhandler, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Win_create_keyval( MPI_Win_copy_attr_function * win_copy_attr_fn, MPI_Win_delete_attr_function * win_delete_attr_fn, int * win_keyval, void * extra_state ) {return MPI_SUCCESS;}
//~void mpi_win_create_keyval_( MPI_Win_copy_attr_function * win_copy_attr_fn, MPI_Win_delete_attr_function * win_delete_attr_fn, int * win_keyval, void * extra_state, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Win_delete_attr( MPI_Win win, int win_keyval ) {return MPI_SUCCESS;}
//~void mpi_win_delete_attr_( MPI_Win win, int win_keyval, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Win_detach( MPI_Win win, void * base ) {return MPI_SUCCESS;}
//~void mpi_win_detach_( MPI_Win win, void * base, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Win_fence( int assert, MPI_Win win ) {return MPI_SUCCESS;}
//~void mpi_win_fence_( int assert, MPI_Win win, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Win_flush( int rank, MPI_Win win ) {return MPI_SUCCESS;}
//~void mpi_win_flush_( int rank, MPI_Win win, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Win_flush_all( MPI_Win win ) {return MPI_SUCCESS;}
//~void mpi_win_flush_all_( MPI_Win win, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Win_flush_local( int rank, MPI_Win win ) {return MPI_SUCCESS;}
//~void mpi_win_flush_local_( int rank, MPI_Win win, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Win_flush_local_all( MPI_Win win ) {return MPI_SUCCESS;}
//~void mpi_win_flush_local_all_( MPI_Win win, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Win_free( MPI_Win * win ) {return MPI_SUCCESS;}
//~void mpi_win_free_( MPI_Win * win, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Win_free_keyval( int * win_keyval ) {return MPI_SUCCESS;}
//~void mpi_win_free_keyval_( int * win_keyval, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Win_get_attr( MPI_Win win, int win_keyval, void * attribute_val, int * flag ) {return MPI_SUCCESS;}
//~void mpi_win_get_attr_( MPI_Win win, int win_keyval, void * attribute_val, int * flag, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Win_get_errhandler( MPI_Win win, MPI_Errhandler * errhandler ) {return MPI_SUCCESS;}
//~void mpi_win_get_errhandler_( MPI_Win win, MPI_Errhandler * errhandler, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Win_get_group( MPI_Win win, MPI_Group * group ) {return MPI_SUCCESS;}
//~void mpi_win_get_group_( MPI_Win win, MPI_Group * group, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Win_get_info( MPI_Win win, MPI_Info * info_used ) {return MPI_SUCCESS;}
//~void mpi_win_get_info_( MPI_Win win, MPI_Info * info_used, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Win_get_name( MPI_Win win, char * win_name, int * resultlen ) {return MPI_SUCCESS;}
//~void mpi_win_get_name_( MPI_Win win, char * win_name, int * resultlen, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Win_lock( int lock_type, int rank, int assert, MPI_Win win ) {return MPI_SUCCESS;}
//~void mpi_win_lock_( int lock_type, int rank, int assert, MPI_Win win, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Win_lock_all( int assert, MPI_Win win ) {return MPI_SUCCESS;}
//~void mpi_win_lock_all_( int assert, MPI_Win win, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Win_post( MPI_Group group, int assert, MPI_Win win ) {return MPI_SUCCESS;}
//~void mpi_win_post_( MPI_Group group, int assert, MPI_Win win, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Win_set_attr( MPI_Win win, int win_keyval, void * attribute_val ) {return MPI_SUCCESS;}
//~void mpi_win_set_attr_( MPI_Win win, int win_keyval, void * attribute_val, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Win_set_errhandler( MPI_Win win, MPI_Errhandler errhandler ) {return MPI_SUCCESS;}
//~void mpi_win_set_errhandler_( MPI_Win win, MPI_Errhandler errhandler, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Win_set_info( MPI_Win win, MPI_Info info ) {return MPI_SUCCESS;}
//~void mpi_win_set_info_( MPI_Win win, MPI_Info info, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Win_set_name( MPI_Win win, const char * win_name ) {return MPI_SUCCESS;}
//~void mpi_win_set_name_( MPI_Win win, const char * win_name, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Win_shared_query( MPI_Win win, int rank, MPI_Aint * size, int * disp_unit, void * baseptr ) {return MPI_SUCCESS;}
//~void mpi_win_shared_query_( MPI_Win win, int rank, MPI_Aint * size, int * disp_unit, void * baseptr, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Win_start( MPI_Group group, int assert, MPI_Win win ) {return MPI_SUCCESS;}
//~void mpi_win_start_( MPI_Group group, int assert, MPI_Win win, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Win_sync( MPI_Win win ) {return MPI_SUCCESS;}
//~void mpi_win_sync_( MPI_Win win, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Win_test( MPI_Win win, int * flag ) {return MPI_SUCCESS;}
//~void mpi_win_test_( MPI_Win win, int * flag, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Win_unlock( int rank, MPI_Win win ) {return MPI_SUCCESS;}
//~void mpi_win_unlock_( int rank, MPI_Win win, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Win_unlock_all( MPI_Win win ) {return MPI_SUCCESS;}
//~void mpi_win_unlock_all_( MPI_Win win, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_Win_wait( MPI_Win win ) {return MPI_SUCCESS;}
//~void mpi_win_wait_( MPI_Win win, int * ierror ) {*ierror = MPI_SUCCESS;}

//~double MPI_Wtick( ) {return 0.0;}
//~double mpi_wtick_( ) {return 0.0;}

//~double MPI_Wtime() {return 0.0;}
//~double mpi_wtime_() {return 0.0;}

//~int MPI_T_init_thread( int required, int * provided ) {return MPI_SUCCESS;}
//~void mpi_t_init_thread_( int required, int * provided, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_T_finalize( ) {return MPI_SUCCESS;}
//~void mpi_t_finalize_( int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_T_cvar_get_num( int * num_cvar ) {return MPI_SUCCESS;}
//~void mpi_t_cvar_get_num_( int * num_cvar, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_T_cvar_get_info( int cvar_index, char * name, int * name_len, int * verbosity, MPI_Datatype * datatype, MPI_T_enum * enumtype, char * desc, int * desc_len, int * bind, int * scope ) {return MPI_SUCCESS;}
//~void mpi_t_cvar_get_info_( int cvar_index, char * name, int * name_len, int * verbosity, MPI_Datatype * datatype, MPI_T_enum * enumtype, char * desc, int * desc_len, int * bind, int * scope, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_T_cvar_get_index( const char * name, int * cvar_index ) {return MPI_SUCCESS;}
//~void mpi_t_cvar_get_index_( const char * name, int * cvar_index, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_T_cvar_handle_alloc( int cvar_index, void * obj_handle, MPI_T_cvar_handle * handle, int * count ) {return MPI_SUCCESS;}
//~void mpi_t_cvar_handle_alloc_( int cvar_index, void * obj_handle, MPI_T_cvar_handle * handle, int * count, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_T_cvar_handle_free( MPI_T_cvar_handle * handle ) {return MPI_SUCCESS;}
//~void mpi_t_cvar_handle_free_( MPI_T_cvar_handle * handle, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_T_cvar_read( MPI_T_cvar_handle handle, void * buf ) {return MPI_SUCCESS;}
//~void mpi_t_cvar_read_( MPI_T_cvar_handle handle, void * buf, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_T_cvar_write( MPI_T_cvar_handle handle, const void * buf ) {return MPI_SUCCESS;}
//~void mpi_t_cvar_write_( MPI_T_cvar_handle handle, const void * buf, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_T_category_get_num( int * num_cat ) {return MPI_SUCCESS;}
//~void mpi_t_category_get_num_( int * num_cat, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_T_category_get_info( int cat_index, char * name, int * name_len, char * desc, int * desc_len, int * num_cvars, int * num_pvars, int * num_categories ) {return MPI_SUCCESS;}
//~void mpi_t_category_get_info_( int cat_index, char * name, int * name_len, char * desc, int * desc_len, int * num_cvars, int * num_pvars, int * num_categories, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_T_category_get_index( const char * name, int * category_index ) {return MPI_SUCCESS;}
//~void mpi_t_category_get_index_( const char * name, int * category_index, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_T_category_get_cvars( int cat_index, int len, int indices[] ) {return MPI_SUCCESS;}
//~void mpi_t_category_get_cvars_( int cat_index, int len, int indices[], int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_T_category_get_pvars( int cat_index, int len, int indices[] ) {return MPI_SUCCESS;}
//~void mpi_t_category_get_pvars_( int cat_index, int len, int indices[], int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_T_category_get_categories( int cat_index, int len, int indices[] ) {return MPI_SUCCESS;}
//~void mpi_t_category_get_categories_( int cat_index, int len, int indices[], int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_T_category_changed( int * stamp ) {return MPI_SUCCESS;}
//~void mpi_t_category_changed_( int * stamp, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_T_pvar_get_num( int * num_pvar ) {return MPI_SUCCESS;}
//~void mpi_t_pvar_get_num_( int * num_pvar, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_T_pvar_get_info( int pvar_index, char * name, int * name_len, int * verbosity, int * var_class, MPI_Datatype * datatype, MPI_T_enum * enumtype, char * desc, int * desc_len, int * bind, int * readonly, int * continuous, int * atomic ) {return MPI_SUCCESS;}
//~void mpi_t_pvar_get_info_( int pvar_index, char * name, int * name_len, int * verbosity, int * var_class, MPI_Datatype * datatype, MPI_T_enum * enumtype, char * desc, int * desc_len, int * bind, int * readonly, int * continuous, int * atomic, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_T_pvar_get_index( const char * name, int * pvar_index ) {return MPI_SUCCESS;}
//~void mpi_t_pvar_get_index_( const char * name, int * pvar_index, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_T_pvar_session_create( MPI_T_pvar_session * session ) {return MPI_SUCCESS;}
//~void mpi_t_pvar_session_create_( MPI_T_pvar_session * session, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_T_pvar_session_free( MPI_T_pvar_session * session ) {return MPI_SUCCESS;}
//~void mpi_t_pvar_session_free_( MPI_T_pvar_session * session, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_T_pvar_handle_alloc( MPI_T_pvar_session session, int pvar_index, void * obj_handle, MPI_T_pvar_handle * handle, int * count ) {return MPI_SUCCESS;}
//~void mpi_t_pvar_handle_alloc_( MPI_T_pvar_session session, int pvar_index, void * obj_handle, MPI_T_pvar_handle * handle, int * count, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_T_pvar_handle_free( MPI_T_pvar_session session, MPI_T_pvar_handle * handle ) {return MPI_SUCCESS;}
//~void mpi_t_pvar_handle_free_( MPI_T_pvar_session session, MPI_T_pvar_handle * handle, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_T_pvar_start( MPI_T_pvar_session session, MPI_T_pvar_handle handle ) {return MPI_SUCCESS;}
//~void mpi_t_pvar_start_( MPI_T_pvar_session session, MPI_T_pvar_handle handle, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_T_pvar_stop( MPI_T_pvar_session session, MPI_T_pvar_handle handle ) {return MPI_SUCCESS;}
//~void mpi_t_pvar_stop_( MPI_T_pvar_session session, MPI_T_pvar_handle handle, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_T_pvar_read( MPI_T_pvar_session session, MPI_T_pvar_handle handle, void * buf ) {return MPI_SUCCESS;}
//~void mpi_t_pvar_read_( MPI_T_pvar_session session, MPI_T_pvar_handle handle, void * buf, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_T_pvar_write( MPI_T_pvar_session session, MPI_T_pvar_handle handle, const void * buf ) {return MPI_SUCCESS;}
//~void mpi_t_pvar_write_( MPI_T_pvar_session session, MPI_T_pvar_handle handle, const void * buf, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_T_pvar_reset( MPI_T_pvar_session session, MPI_T_pvar_handle handle ) {return MPI_SUCCESS;}
//~void mpi_t_pvar_reset_( MPI_T_pvar_session session, MPI_T_pvar_handle handle, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_T_pvar_readreset( MPI_T_pvar_session session, MPI_T_pvar_handle handle, void * buf ) {return MPI_SUCCESS;}
//~void mpi_t_pvar_readreset_( MPI_T_pvar_session session, MPI_T_pvar_handle handle, void * buf, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_T_enum_get_info( MPI_T_enum enumtype, int * num, char * name, int * name_len ) {return MPI_SUCCESS;}
//~void mpi_t_enum_get_info_( MPI_T_enum enumtype, int * num, char * name, int * name_len, int * ierror ) {*ierror = MPI_SUCCESS;}

//~int MPI_T_enum_get_item( MPI_T_enum enumtype, int index, int * value, char * name, int * name_len ) {return MPI_SUCCESS;}
//~void mpi_t_enum_get_item_( MPI_T_enum enumtype, int index, int * value, char * name, int * name_len, int * ierror ) {*ierror = MPI_SUCCESS;}
