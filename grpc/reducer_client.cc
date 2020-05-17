#include "reducer_client.h"
#include "../tools/data.h"
#include <inttypes.h>

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

int32_t ReducerClient::reducer_init(char* application, int32_t n_threads, char* in, char* out, char* args, int32_t barrier,int32_t id) {
    ReducerInitRequest request;
    ReducerInitReply reply;
    ClientContext context;

    request.set_application(application);
    request.set_threads(n_threads);
    request.set_inpath(in);
    request.set_outpath(out);
    request.set_args(args);
    request.set_barrier(barrier);
    request.set_id(id);

    Status status = stub_->reducer_init(&context, request, &reply);

    if(status.ok()){
        int32_t ret = reply.result();
        return ret;
    } else {
        return -1;
    }
}

result_pair ReducerClient::reducer_start(char* application, int32_t n_threads, char* in, char* out, char* args, int32_t barrier,int32_t id) {
    ReducerStartRequest request;
    ReducerStartReply reply;
    ClientContext context;

    request.set_application(application);
    request.set_threads(n_threads);
    request.set_inpath(in);
    request.set_outpath(out);
    request.set_args(args);
    request.set_barrier(barrier);
    request.set_id(id);

    Status status = stub_->reducer_start(&context, request, &reply);



    if(status.ok()){
        result_pair ret(reply.id(), reply.result());
        return ret;
    } else {
        std::cout << status.error_code() << ": " << status.error_message() << std::endl;
        result_pair ret(-1, -1);
        return ret;
    }
}

int ReducerClient::flush(int id, char* buffer, int size) {
    ReducerFlushRequest request;
    ReducerFlushReply reply;
    ClientContext context;

    request.set_id(id);
    request.set_buffer(buffer, size);
    request.set_size(size);

    Status status = stub_->flush(&context, request, &reply);

    if(status.ok()){
        return reply.successful();
    } else {
        return -1;
    }
}

int32_t reducer_init_rpc(struct map_reduce *mr, char* inpath, char* outpath, int32_t id, std::string address) {
    ReducerClient client(
        grpc::CreateChannel(
            address, 
            grpc::InsecureChannelCredentials()
        )
    );
    int32_t response = client.reducer_init(mr->application, mr->n_threads, inpath, outpath, mr->args, mr->barrier, id);
    return response;
}

result_pair reducer_start_rpc(struct map_reduce *mr, char* inpath, char* outpath, int32_t id, std::string address) {
    ReducerClient client(
        grpc::CreateChannel(
            address, 
            grpc::InsecureChannelCredentials()
        )
    );

    result_pair response;
    response = client.reducer_start(mr->application, mr->n_threads, inpath, outpath, mr->args, mr->barrier, id);
    return response;
}