#ifndef WRAPPER_MPI_H
#define WRAPPER_MPI_H

#include "../ccan/ccan/htable/htable.h"
#include "../ccan/ccan/hash/hash.h"
#include <mpi.h>
#include <string.h>
#include <stdio.h>

//#define DEBUG

#ifdef DEBUG
FILE *comm_fd = NULL;

void print_calling_function() {
}

void print_calling_function_special( const char * caller_name )
{
  fprintf(comm_fd, "%s %s\n", caller_name, getenv("MPIRE_ACTIVE_DUMP"));
  fprintf(stderr, "In %s (Dump status = %s)\n", caller_name, getenv("MPIRE_ACTIVE_DUMP"));
}

#define print_calling_function(void) print_calling_function_special(__func__)
#endif


static struct htable requestHtab;

/*Global variable defined in MPI_Init()*/
int MPIre_rank; //MPI rank to capture

typedef struct
{
  MPI_Request* request;
  MPI_Datatype datatype;
  void * buffer;
  int bufsize;
} request_t;

bool streq(const void * e, void * ptr) 
{
  return ((request_t *)e)->request == ptr;
}

void print_hash_table()
{
  struct htable_iter iter;
  request_t *p;
  for (p = htable_first(&requestHtab,&iter); p; p = htable_next(&requestHtab, &iter))
    fprintf(stderr, "MPI Capture: print_hash_table() %p\n", p->request);
}

size_t rehash(const void * e, void * unused) 
{
  return hash_pointer(((request_t*)e)->request, 0);
}

void add_request(void * newBuffer, int newSize, MPI_Datatype newDatatype, void * newRequest) {
  size_t hash_key = hash_pointer(newRequest, 0);
  request_t * newEntry = NULL;

  newEntry = htable_get(&requestHtab, hash_key, streq, newRequest);

  if(newEntry != NULL) {
    fprintf(stderr, "ERROR: this request is already in the htable\n");
    exit(EXIT_FAILURE);
  }

  newEntry = malloc(sizeof(request_t));

  newEntry->request  = newRequest;
  newEntry->buffer   = newBuffer;
  newEntry->datatype = newDatatype;
  newEntry->bufsize  = newSize;

  htable_add(&requestHtab, hash_key, newEntry);
  #ifdef DEBUG
    fprintf(stderr, "Request %p successfully added\n", newRequest);
  #endif
}

request_t* get_request(void* request) {
  return htable_get(&requestHtab, hash_pointer(request, 0), streq, request);
}

int del_request(struct htable* ht, size_t hash, request_t* ptr) {
  int res = htable_del(ht, hash, ptr);
  if (!res) {
    fprintf(stderr, "ERROR: Failed to delete request %p!\n", ptr->request);
    exit(EXIT_FAILURE);
  }
}

#endif /*WRAPPER_MPI_H*/
