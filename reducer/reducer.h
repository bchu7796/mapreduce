#ifndef REDUCER_H
#define REDUCER_H

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
#include "../mapreduce/mapreduce.h"

#include <grpcpp/grpcpp.h>
#include "../reducer.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using reducer::Reducer;
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

class ReducerServiceImplementation final : public Reducer::Service {
    Status reducer_init(
        ServerContext* context, 
        const ReducerInitRequest* request, 
        ReducerInitReply* reply
    ) override;

    Status reducer_start(
        ServerContext* context, 
        const ReducerStartRequest* request, 
        ReducerStartReply* reply
    ) override;

    Status flush(
        ServerContext* context, 
        const ReducerFlushRequest* request, 
        ReducerFlushReply* reply
    ) override;

    private:
        int32_t reducer_start_(struct map_reduce *mr, int id);
        int32_t receiver(struct map_reduce *mr, std::string buffer, int size, int id);
        reducer_struct* reducer_init(struct map_reduce *mr, int id);
        void reducer_destroy(struct reducer_struct *reduce_worker);
        struct map_reduce* mr;
};


#endif


