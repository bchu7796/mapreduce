/* Header includes */
#include "mapper.h"
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
#include <vector>
#include <string>
#include <fstream>
#include <streambuf>
#include <sstream>
#include <cstdlib> 
#include "../mapreduce/mapreduce.h"
#include "../tools/data.h"
#include "../tools/util.h"
#include "../grpc/reducer_client.h"

#include <grpcpp/grpcpp.h>
#include "../mapper.grpc.pb.h"
#include "../reducer.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using mapper::Mapper;
using mapper::MapperRequest;
using mapper::MapperReply;

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using reducer::Reducer;
using reducer::ReducerInitRequest;
using reducer::ReducerInitReply;
using reducer::ReducerStartRequest;
using reducer::ReducerStartReply;
using reducer::ReducerFlushRequest;
using reducer::ReducerFlushReply;


#define MAPPER_BUFFER_SIZE 1024

static std::vector<std::string> read_tokens(mapper_struct* map_worker) {
    /**TODO
     * 
     * READ FROM HDFS
     * 
     */
    std::vector<std::string> a;
    return a;
}

void* map_wrapper(void* map_worker) {
    // turn the argument to mapper_struct type
    mapper_struct *map_exec = (mapper_struct*) map_worker;
    printf("mapper(%d) start map_wrapper\n", map_exec->id);
    // Generate tokens for map function
    std::vector<std::string> tokens = read_tokens(map_exec);
    // Pass each token to the map function through single_pair
    kvpair* single_pair = (kvpair*) malloc(sizeof(kvpair));
    for(int i = 0; i < tokens.size(); i++) {
        // assign keypair's key
        single_pair->key = malloc((tokens[i].size()) * sizeof(char));
        memcpy(single_pair->key, tokens[i].c_str(), (tokens[i].size()));
        // We have to assign keypair's value for grep
        if(!strcmp(map_exec->mr->application, "grep")) {
            single_pair->value = malloc(strlen(map_exec->mr->args) * sizeof(char));
            memcpy(single_pair->value, map_exec->mr->args, strlen(map_exec->mr->args));
            single_pair->valuesz = strlen(map_exec->mr->args);
        }
        single_pair->keysz = tokens[i].size();
        map_exec->mr->map_fnc(map_exec->mr, single_pair);
        free(single_pair->key);
        free(single_pair->value);
    }
    // free single_pair's memory
    free(single_pair);
    // tell the sender this mapper has finished
    // set the map_status to 0
    // and tell the sender we put something
    // inside the buffer
    map_exec->finished = 1;
    map_exec->mr->mapfn_status[map_exec->id] = 0;
    pthread_cond_signal(&map_exec->mr->not_empty);
    printf("mapper(%d) finish map wrapper\n", map_exec->id);
    return NULL;
}


Status MapperServiceImplementation::mapper_start(
    ServerContext* context, 
    const MapperRequest* request, 
    MapperReply* reply
) {
    char *app = (char*) request->application().c_str();
    int n_threads = request->threads();
    char *in = (char*) request->inpath().c_str();
    char *out = (char*) request->outpath().c_str();
    char *args = (char*) request->args().c_str();
    int id_num = request->id();

    printf("mapper(%d) rpc call to mapper\n", id_num);

    // initize mr_init with arguments
    map_reduce *mr = mr_init(app, n_threads, in, out, args);

    // start mapper
    mapper_start_(mr, id_num);
    int result = mr->mapfn_status[id_num];

    // reply message
    reply->set_id(id_num);
    reply->set_result(result);

    // destroy mr
    mr_destroy(mr);

    printf("mapper(%d) rpc call returned\n", id_num);
    return Status::OK;
}

int32_t MapperServiceImplementation::mapper_start_(map_reduce *mr, int32_t id) {
    printf("mapper(%d) start running\n", id);
    // create map_struct instance
    mapper_struct *map_worker = mapper_init(mr, id);
    // create mapper thread (produce)
    pthread_t map_thread;
    if(pthread_create(&map_thread, NULL, &map_wrapper, (void *)map_worker) != 0) {
        perror("Failed to create map thread.\n");
        return -1;
    }
    // sender thread (flush)
    int32_t sender_return;
    while(!map_worker->finished) {
        sender_return = sender(map_worker, id);
        if(sender_return == 1) {
            break;
        }
    }
    // wait for mapper thread to finish
    pthread_join(map_thread, NULL);
    // make sure mapper thread send a zero
    // if ret == 0, the reducer hasn't get a zero
    // so we send it again
    while(sender_return == 0) {
        printf("mapper(%d) sending buffer to reducer, with buffer size: %d\n", id, mr->used_size);
        sender_return = flush_rpc(mr, id, mr->reducer_address);
    }
    // destroy mapper_struct instance
    mapper_destroy(map_worker);
    printf("mapper(%d) finish\n", id);
    return 0;
}

int32_t MapperServiceImplementation::sender(mapper_struct* map_worker, int id) {
    // lock so that it can synchronize with producer
    pthread_mutex_lock(&map_worker->mr->lock);
    int32_t ret = 0;
    // if no content in the buffer
    while(map_worker->mr->used_size <= 0) {
        // if map_finished, we used flush+rpc to send
        // finish message to reducer
        if(map_worker->finished) {
            printf("mapper(%d) receive finish status\n", id);
            // keep flushing the buffer until reducer receive finish
            while(ret == 0) {
                printf("mapper(%d) sending buffer to reducer, with buffer size: %d\n", id, map_worker->mr->used_size);
                ret = flush_rpc(map_worker->mr, id, map_worker->mr->reducer_address);
            }
            // send this signal so that producer will not be stuck
            pthread_cond_signal(&map_worker->mr->not_full);
            pthread_mutex_unlock(&map_worker->mr->lock);
            return ret;
        }
        // if not finish, wait for producer to produce something
        pthread_cond_wait(&map_worker->mr->not_empty, &map_worker->mr->lock);
    }
    // now there is something in the buffer
    // RPC call to reducer
    printf("mapper(%d) sending buffer to reducer, with buffer size: %d\n", id, map_worker->mr->used_size);
    if(map_worker->mr->used_size > MAPPER_BUFFER_SIZE * MR_BUFFER_THRESHOLD) {
        ret = flush_rpc(map_worker->mr, id, map_worker->mr->reducer_address);
    }

    pthread_cond_signal(&map_worker->mr->not_full);
    pthread_mutex_unlock(&map_worker->mr->lock);
    return ret;
}

mapper_struct* MapperServiceImplementation::mapper_init(map_reduce *mr, int32_t id) {
    printf("mapper(%d) initializing mapper\n", id);
    // create new mapper_struct instance
    mapper_struct *map_worker = (mapper_struct*) malloc(sizeof(mapper_struct));
    if(map_worker == 0) {
        free(map_worker);
        return NULL;
    }
    // initialize local buffer for mapper
    mr->map_buffer          = (char*) malloc(MAPPER_BUFFER_SIZE * sizeof(char));
    mr->used_size           = 0;
    // initialize mapper's status
    mr->mapfn_status = (int*) malloc(mr->n_threads * sizeof(int));
    for(int i = 0; i < mr->n_threads; i++) {
        mr->mapfn_status[i] = -1;
    }
    // initialize mapper struct
    map_worker->id          = id;
    map_worker->mr          = mr;
    map_worker->finished    = 0;
    printf("mapper(%d) done with initialization\n", id);
    return map_worker;
}

void MapperServiceImplementation::mapper_destroy(mapper_struct *map_worker) {
    printf("mapper(%d) destroyed\n", map_worker->id);
    free(map_worker);
}

int32_t MapperServiceImplementation::flush_rpc(map_reduce *mr, int32_t id, std::string address) {
    ReducerClient client(
        grpc::CreateChannel(
            address, 
            grpc::InsecureChannelCredentials()
        )
    );
    int32_t response;
    response = client.flush(id, mr->map_buffer, mr->used_size);
    mr->used_size = 0;
    return response;
}

int32_t main(int32_t argc, char** argv) {
    std::string address("0.0.0.0:5000");
    MapperServiceImplementation service;

    ServerBuilder builder;

    builder.AddListeningPort(address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on port: " << address << std::endl;
    
    server->Wait();
    return 0;
}
