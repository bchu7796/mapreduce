#ifndef REDUCER_CLIENT_H
#define REDUCER_CLIENT_H

#include <inttypes.h>
#include <grpcpp/grpcpp.h>
#include "../reducer.grpc.pb.h"
#include "../tools/data.h"

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

class ReducerClient {
    public:
    ReducerClient(std::shared_ptr<Channel> channel) : stub_(Reducer::NewStub(channel)) {}
    int32_t reducer_init(char* application, int32_t n_threads, char* in, char* out, char* args, int32_t barrier,int32_t id);
    result_pair reducer_start(char* application, int32_t n_threads, char* in, char* out, char* args, int32_t barrier,int32_t id);
    int32_t flush(int32_t id, char* buffer, int32_t size);

    private:
        std::unique_ptr<Reducer::Stub> stub_;
};

int32_t reducer_init_rpc(struct map_reduce *mr, char* inpath, char* outpath, int32_t id, std::string address);
result_pair reducer_start_rpc(struct map_reduce *mr, char* inpath, char* outpath, int32_t id, std::string address);


#endif