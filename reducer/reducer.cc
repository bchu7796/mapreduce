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
#include <grpcpp/grpcpp.h>

#include "../mapreduce/mapreduce.h"
#include "../reducer.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using reducer::ReducerInitRequest;
using reducer::ReducerInitReply;
using reducer::ReducerStartRequest;
using reducer::ReducerStartReply;
using reducer::ReducerFlushRequest;
using reducer::ReducerFlushReply;

#define REDUCE_BUFFER_SIZE 1024000

struct reducer_struct {
    int id;
    struct map_reduce *mr;
    int barrier;
    char* outpath;
    int (*reduce_fnc)(struct map_reduce*, struct kvpair*, size_t num);
};

static void* reduce_wrapper(void* reduce_worker) {
    printf("reducer start map_wrapper\n");
    // type cast the void* to reducer_struct* so that
    // we can access its value
    struct reducer_struct *reduce_exec = (struct reducer_struct*) reduce_worker;
    // variables used to pass into mr_consume
    struct kvpair *kvset;
    size_t *num;   
    // check_mapper_status return 1 if all the mapper
    // send a 0 to reducer
    // keep consuming until all mapper finish working
    while(!check_mapper_status(reduce_exec->mr)) {
        mr_consume(reduce_exec->mr, kvset, num, reduce_exec->barrier);
    }
    // give num 0 to tell application the end of reducer
    reduce_exec->reduce_fnc(reduce_exec->mr, kvset, 0);
    // reducefn status set to 0 (mark it as complete)
    reduce_exec->mr->reducefn_status = 0;
    printf("reducer finish map_wrapper\n");
    return NULL;
}

int32_t ReducerServiceImplementation::check_mapper_status(struct map_reduce *mr) {
    for(int i = 0; i < mr->n_threads; i++) {
        if(mr->mapfn_status[i] != 0){
            printf("check_mapper_status() return 0\n");
            return 0;
        } 
    }
    printf("check_mapper_status() return 1\n");
    return 1;
}

reducer_struct* ReducerServiceImplementation::reducer_init(struct map_reduce *mr, int id) {
    printf("Reducer initializing reduce worker\n");
    
    // create new reduce_struct instance
    struct reducer_struct *reduce_worker = (struct reducer_struct*) malloc(sizeof(struct reducer_struct));
    if(reduce_worker == 0) {
        free(reduce_worker);
        return NULL;
    }
    // initialize reduce buffer
    mr->reduce_buffer = (char*) malloc(REDUCE_BUFFER_SIZE * sizeof(char));
    if(mr->reduce_buffer == 0) {
        free(mr->reduce_buffer);
        printf("failed to allocate buffer for reducer\n");
        return NULL;
    }
    mr->used_size = 0;
    // initialize mapfn_status
    mr->mapfn_status = (int*) malloc(mr->n_threads * sizeof(int));
    if(mr->mapfn_status == 0) {
        free(mr->mapfn_status);
        printf("failed to allocate mapfn_status for reducer\n");
        return NULL;
    }
    for(int i = 0; i < mr->n_threads; i++) {
        mr->mapfn_status[i] = -1;
    }
    // initialize reducer_struct
    reduce_worker->mr = mr;
    reduce_worker->id = id;
    reduce_worker->barrier = mr->barrier;
    reduce_worker->outpath = mr->outpath;
    reduce_worker->reduce_fnc = mr->reduce_fnc;
    printf("initialization of reduce_worker with id: %d done\n", id);
    return reduce_worker;
}

void ReducerServiceImplementation::reducer_destroy(struct reducer_struct *reduce_worker) {
    free(reduce_worker);
}

int32_t ReducerServiceImplementation::receiver(struct map_reduce *mr, std::string buffer, int size, int id) {
    pthread_mutex_lock(&mr->lock);
    int ret = 0;
    printf("receive a buffer from mapper(%d), the size of buffer is %d bytes\n", id, size);
    // if size of received buffer = 0, the map function sending a finish signal.
    if(size == 0) {
        printf("mapper(%d) has finished\n", id);
        mr->mapfn_status[id] = 0;
        ret = 1;
        // signal reducer all mappers has finished
        if(check_mapper_status(mr) != 0) {
            printf("signal comsumer all mappers has finished\n");
            mr->receiver_finished = 1;
            pthread_cond_signal(&mr->map_finished);
            pthread_cond_signal(&mr->not_empty);
            pthread_mutex_unlock(&mr->lock);
            return ret;
        }
    }
    // otherwise, move the received buffer to reduce_buffer.
    else {
        // First check if the buffer is overflow
        // if it is full, wait for consumer to send signal
        while((mr->used_size + size) >= REDUCE_BUFFER_SIZE) {
            pthread_cond_wait(&mr->not_full, &mr->lock);
        }
        // move the content from received buffer to reduce buffer
        printf("flushing buffer for mapper(%d)\n", id);
        memmove(&mr->reduce_buffer[mr->used_size], buffer.c_str(), size);
        mr->used_size += size;
        printf("finish flushing buffer for mapper(%d)\n", id);
    }
    // tell the reducer thread the buffer is not empty (consumer)
    pthread_cond_signal(&mr->not_empty);
    printf("flush successful\n");
    pthread_mutex_unlock(&mr->lock);
    return ret;
}

int32_t ReducerServiceImplementation::reducer_start_(struct map_reduce *mr, int id) {
    printf("reducer start running\n");
    // create a reducer_struct instance
    struct reducer_struct *reduce_worker = reducer_init(mr, id);
    reduce_wrapper((void *)reduce_worker);
    // destroy reducer_struct instance
    reducer_destroy(reduce_worker);
    printf("reducer finish running\n");
    return 0;
}


Status ReducerServiceImplementation::reducer_init(
    ServerContext* context, 
    const ReducerInitRequest* request, 
    ReducerInitReply* reply
) override {
    printf("rpc call to reduce_init\n");
    char *app = (char*) request->application().c_str();
    int n_threads = request->threads();
    char *in = (char*) request->inpath().c_str();
    char *out = (char*) request->outpath().c_str();
    char *args = (char*) request->args().c_str();
    int id_num = request->id();

    mr = mr_init(app, n_threads, in, out, args);

    mr->barrier = request->barrier();
    reply->set_result(0);

    return Status::OK;
}

Status ReducerServiceImplementation::reducer_start(
    ServerContext* context, 
    const ReducerStartRequest* request, 
    ReducerStartReply* reply
) override {
    printf("call to reducer rpc\n");
    char *app = (char*) request->application().c_str();
    int n_threads = request->threads();
    char *in = (char*) request->inpath().c_str();
    char *out = (char*) request->outpath().c_str();
    char *args = (char*) request->args().c_str();
    int id_num = request->id();

    // mr = mr_init(app, n_threads, in, out, args);
    reducer_start_(mr, id_num);
    int result = mr->reducefn_status;
    reply->set_id(id_num);
    reply->set_result(result);

    mr_destroy(mr);
    printf("return from reducer rpc\n");
    return Status::OK;
} 

Status ReducerServiceImplementation::flush(
    ServerContext* context, 
    const ReducerFlushRequest* request, 
    ReducerFlushReply* reply
) override {
    int finished = receiver(mr, request->buffer(), request->size(), request->id());
    
    reply->set_successful(finished);
    return Status::OK;
} 

int main(int argc, char** argv) {
    std::string address("0.0.0.0:5001");
    ReducerServiceImplementation service;

    ServerBuilder builder;

    builder.AddListeningPort(address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on port: " << address << std::endl;

    server->Wait();
    return 0;
}


