/* Header includes */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <fcntl.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <sstream>
#include <algorithm>
#include <assert.h>
#include <grpcpp/grpcpp.h>

#include "mapreduce.h"
#include "../tools/data.h"
#include "../tools/util.h"
#include "../grpc/mapper_client.h"
#include "../grpc/reducer_client.h"
#include "../mapper.grpc.pb.h"
#include "../reducer.grpc.pb.h"

#define MAPPER_BUFFER_SIZE 10240

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using grpc::CompletionQueue;

using mapper::Mapper;
using mapper::MapperRequest;
using mapper::MapperReply;

using reducer::Reducer;
using reducer::ReducerInitRequest;
using reducer::ReducerInitReply;
using reducer::ReducerStartRequest;
using reducer::ReducerStartReply;
using reducer::ReducerFlushRequest;
using reducer::ReducerFlushReply;

static void* map_wrapper(void* args) {
    // Reconstruct the Arguments
    struct rpc_args *arguments = (struct rpc_args *) args;
    // Call the map rpc and save the return value
    std::string address = arguments->mr->mapper_addresses[arguments->id];
    result_pair ret = mapper_start_rpc(arguments->mr, arguments->inpath, arguments->outpath, arguments->id, address);
    // Check return value
    result_pair error_value(-1, -1);
    if(ret != error_value) {
        printf("mapper_start_rpc() returns error\n");
    }
    arguments->mr->mapfn_status[arguments->id] = ret.result;
    // Free argument
    // we malloc to arguments->inpath in mr_start()
    free(arguments->inpath);
    free(args);
    return NULL;
}

static void* reduce_wrapper(void* args) {
    // Reconstruct the Arguments
    struct rpc_args *arguments = (struct rpc_args *) args;
    // Call the map rpc and save the return value
    std::string address = arguments->mr->reducer_address;
    struct result_pair ret = reducer_start_rpc(arguments->mr, arguments->inpath, arguments->outpath, arguments->id, address);
    // Check return value
    result_pair error_value(-1, -1);
    if(ret != error_value) {
        printf("reducer_start_rpc() returns error\n");
    }
    arguments->mr->reducefn_status = ret.result;
    // free argument
    free(args);
    return NULL;
}

/**
 * Used to load config file to shared file system
 * 
 * 
 */
static int32_t load_config_file() {
    return 0;
}

map_reduce *mr_init(const char *application, int32_t threads, const char *inpath, const char *outpath, const char *helper_args) {
    printf("Initializing mr...\n");
    struct map_reduce *mr = (struct map_reduce *) malloc(sizeof(struct map_reduce));

    if(read_config("mr_config", mr) == -1) {
        printf("Read Config File Failed\n");
        return NULL;
    }

    // check malloc success or not
    if(mr == 0) {
        free(mr);
        return NULL;
    }
    
    // initialize variables with parameters
    assert(strlen(application) <= 99);
    mr->application     = (char*) malloc(100 * sizeof(char));
    mr->application     = strcpy(mr->application, application);
    mr->n_threads       = threads;
    
    // paths
    assert(strlen(inpath) <= 99);
    mr->inpath          = (char*) malloc(100 * sizeof(char));
    mr->inpath          = strcpy(mr->inpath, inpath);
    assert(strlen(outpath) <= 99);
    mr->outpath         = (char*) malloc(100 * sizeof(char));
    mr->outpath         = strcpy(mr->outpath, outpath);
    
    // function pointers
    mr->map_fnc         = &map;
    mr->reduce_fnc      = &reduce;
    
    // status of map_worker and reduce_worker
    // used by mr_finished
    mr->mapfn_status    = (int*) malloc(threads * sizeof(int));
    mr->reducefn_status = -1;
    for(int i = 0; i < threads; i++) {
        mr->mapfn_status[i] = -1;
    }
    mr->receiver_finished = 0;
    
    // Initialize Lock & Conditional Variables
    pthread_mutex_init(&mr->lock, NULL);
    pthread_cond_init(&mr->not_full, NULL);
    pthread_cond_init(&mr->not_empty, NULL);
    pthread_cond_init(&mr->map_finished, NULL);
    
    // Threads
    mr->map_threads     = (pthread_t*) malloc(threads * sizeof(pthread_t));
    
    // BUFFERS are initialized in mapper and reducer
    // malloc 0 for mr_destroy to free
    mr->map_buffer      = (char*) malloc(0);
    mr->reduce_buffer   = (char*) malloc(0);
    
    // Arguments
    assert(strlen(helper_args) <= 99);
    mr->args            = (char*) malloc(100 * sizeof(char));
    mr->args            = strcpy(mr->args, helper_args);

    mr->barrier         = 0;
    mr->genOutput       = 0;

    load_config_file();

    printf("mr initialization done\n");
    return mr;
}

void mr_destroy(struct map_reduce *mr) {
    free(mr->application);
    free(mr->inpath);
    free(mr->outpath);
    free(mr->mapfn_status);
    free(mr->map_threads);
    free(mr->map_buffer);
    free(mr->reduce_buffer);
    free(mr->args);
    free(mr);
}

/**
 * Used to load input file to shared file system
 * 
 * 
 */
static int32_t load_input_file() {

    return 0;
}

int mr_start(struct map_reduce *mr, const int barrierEnable) {
    printf("starting mapper and reducer...\n");
    // store barrierEanble to mr
    mr->barrier = barrierEnable;
    // first initialize reducer
    // because mapper and reducer will start running at the same time
    // if we don't initialize reducer first
    // errors occur
    reducer_init_rpc(mr, mr->inpath, mr->outpath, 0, mr->reducer_address);

    // Load input file to HDFS
    // We don't include directory
    // for HDFS Path
    std::vector<std::string> splits;
    split(mr->inpath, splits, '/');
    std::string HDFS_path = splits.back();
    
    load_input_file();

    // call mapper
    for(int i = 0; i < mr->n_threads; i++) {
        // since each thread has different argument
        // we have to allocate new map_args for each thread
        // args will be freed inside map_wrapper()
        rpc_args* map_args = (rpc_args*) malloc(sizeof(rpc_args));
        if(map_args == 0) {
            printf("failed to allocate map args memory\n");
            return -1;
        }
        map_args->mr = mr;
        
        // since we are not sure if the wrapper will finish before
        // this function return, the string HDFS_path might be destroyed
        // so we malloc a new space and use strcpy here to pass argument
        // the space will be free inside map_wrapper()
        map_args->inpath = (char*) malloc(100 * sizeof(char));
        if(map_args->inpath == 0) {
            printf("failed to allocate map args memory\n");
            return -1;
        }
        strcpy(map_args->inpath, HDFS_path.c_str());
        
        // we don't use outpath in mapper
        // don't need to allocate a new space for outpath
        map_args->outpath = NULL;

        // assign id for each mapper
        map_args->id = i;

        // create threads
        if(pthread_create(&mr->map_threads[i], NULL, &map_wrapper, (void *)map_args) != 0) {
            perror("Failed to create map thread.\n");
            return -1;
        }
    }

    // call reducer
    rpc_args* reduce_args = (rpc_args*) malloc(sizeof(rpc_args));
    if(reduce_args == 0) {
        printf("failed to allocate reduce args memory\n");
        return -1;
    }
    reduce_args->mr = mr;
    // we don't need inpath for reducer
    reduce_args->inpath = NULL;

    // outpath
    reduce_args->outpath = (char*) malloc(100 * sizeof(char));
    if(reduce_args->outpath == 0) {
        printf("failed to allocate reduce args memory\n");
        return -1;
    }
    strcpy(reduce_args->outpath, mr->outpath);

    // id
    reduce_args->id = mr->n_threads;

    // create thread for reducer
    if(pthread_create(&mr->reduce_thread, NULL, &reduce_wrapper, (void *)reduce_args) != 0) {
        perror("Failed to create reducer thread.\n");
        return -1;
    }

    printf("mapper and reducer has been started\n");

    return 0;
}

int mr_finish(struct map_reduce *mr) {
    // wait for all threads we create in mr_start to finish

    // threads for mappers
    for(int i = 0; i < mr->n_threads; i++) {
        if(pthread_join(mr->map_threads[i], NULL)) {
            perror("Failed to wait a map thead end.\n");
            return -1;
        }
    }
    // threads for reducer
    if(pthread_join(mr->reduce_thread, NULL)) {
        perror("Failed to wait a map thead end.\n");
        return -1;
    }
    // check execution status
    // status for mappers
    for(int i = 0; i < mr->n_threads; i++) {
        if (mr->mapfn_status[i] !=  0) return -1;
    }
    // status for reducer
    if(mr->reducefn_status != 0) return -1;
    printf("mr_finished return 0\n");
    // set genOutPut to 1
    mr->genOutput = 1 ;
    // return 0 if no errors
    return 0;
}

int mr_produce(struct map_reduce *mr, const struct kvpair *kv) {
    // Lock so that producer and sender can synchronize
    pthread_mutex_lock(&mr->lock);
    // Use the kv_pair size to check if buffer overflow
    int kv_size = kv->keysz + kv->valuesz + 8;
    // Wait for the sender to send out content in the buffer to reducer
    while((mr->used_size + kv_size) >= MAPPER_BUFFER_SIZE * MR_BUFFER_THRESHOLD) {
        pthread_cond_wait(&mr->not_full, &mr->lock);
    }
    // if not overflow, put the kvpair in the buffer
    // put from the place we last use
    memmove(&mr->map_buffer[mr->used_size], &kv->keysz, 4);
    mr->used_size += 4;
    memmove(&mr->map_buffer[mr->used_size], kv->key, kv->keysz);
    mr->used_size += kv->keysz;
    memmove(&mr->map_buffer[mr->used_size], &kv->valuesz, 4);
    mr->used_size += 4;
    memmove(&mr->map_buffer[mr->used_size], kv->value, kv->valuesz);
    mr->used_size += kv->valuesz;
    // Finish copying kvpair to the memory
    // signal the sender the buffer is not empty
    pthread_cond_signal (&mr->not_empty);
    // Unlock
    pthread_mutex_unlock(&mr->lock);
    // Success
    return 1;
}

int mr_consume(struct map_reduce *mr, struct kvpair *kvset, size_t *num, const int barrierEnable) {
    // lock so that consumer and receiver can synchronize
    pthread_mutex_lock(&mr->lock);
    printf("start consuming...\n");
    // if barrier flag enable
    // consumer have to wait for receiver
    // to get all the finish massages from
    // mappers.
    if(barrierEnable) {
        printf("barrier enable, wait for mappers to finish\n");
        pthread_cond_wait(&mr->map_finished, &mr->lock);
    }
    // Check the size to make sure that
    // there is something in the buffer
    // for us to consume
    while(mr->used_size <= 0) {
        // if this is true
        // it means that all mappers finished
        if(mr->receiver_finished == 1) {
            printf("consumer receive finish status\n");
            pthread_mutex_unlock(&mr->lock);
            return 1;
        }
        // otherwise
        // Wait for signal from receiver
        pthread_cond_wait(&mr->not_empty, &mr->lock);
    }
    // we have to first know how many kvpairs
    // are in the buffer
    int offset = 0;
    (*num) = 0;
    struct kvpair kv;
    while(offset != mr->used_size) {
        memcpy(&kv.keysz, &mr->reduce_buffer[offset], 4);
        offset += 4;
        offset += kv.keysz;
        memcpy(&kv.valuesz, &mr->reduce_buffer[offset], 4);
        offset += 4;
        offset += kv.valuesz;
        (*num)++;
    }
    // Since we can not access the value in the buffer anymore
    // we have to allocate another space for kvpairs
    mr->key_assigned_address = (char*) malloc(mr->used_size * sizeof(char));
    memcpy(mr->key_assigned_address, mr->reduce_buffer, mr->used_size);
    // set to 0, so we can reuse the buffer space
    mr->used_size = 0;
    // after knowing how many kvpairs are in the buffer
    // we can malloc an array of kvpairs at kvset
    // and let pointers point to the space we just assign
    offset = 0;
    kvset = (kvpair*) malloc((*num) * sizeof(kvpair));
    for(int i = 0; i < (*num); i++) {
        memcpy(&kvset[i].keysz, &mr->key_assigned_address[offset], 4);
        offset += 4;
        kvset[i].key = &(mr->key_assigned_address[offset]);
        offset += kvset[i].keysz;
        memcpy(&kvset[i].valuesz, &mr->key_assigned_address[offset], 4);
        offset += 4;
        kvset[i].value = &(mr->key_assigned_address[offset]);
        offset += kvset[i].valuesz;
    }
    // so now we have kvpairs to ready to be processed
    printf("execute reduce function\n");
    mr->reduce_fnc(mr, kvset, *num);
    printf("reduce function return\n");
    // The kvpairs has been processed
    // we can free the memory
    free(mr->key_assigned_address);
    free(kvset);
    // since we consume the buffer
    // we can tell receiver the buffer
    // is not full
    pthread_cond_signal(&mr->not_full);
    pthread_mutex_unlock(&mr->lock);
    printf("finish consuming...\n");
    // Success
    return 1;
}

static int32_t write_output_file() {

    return 0;
}

int mr_output(struct map_reduce *mr, char *writeBuffer, size_t bufferLength) {
    
    write_output_file();

    return 0;
}
