#include "mapper_client.h"
#include "../tools/data.h"
#include <inttypes.h>

using mapper::Mapper;
using mapper::MapperRequest;
using mapper::MapperReply;

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;


result_pair MapperClient::mapper_start(char* application, int32_t n_threads, char* in, char* out, char* args, int32_t id) {
    MapperRequest request;
    MapperReply reply;
    ClientContext context;

    request.set_application(application);
    request.set_threads(n_threads);
    request.set_inpath(in);
    request.set_outpath(out);
    request.set_args(args);
    request.set_id(id);

    Status status = stub_->mapper_start(&context, request, &reply);

    if(status.ok()){
        result_pair ret(reply.id(), reply.result());
        return ret;
    } else {
        std::cout << status.error_code() << ": " << status.error_message() << std::endl;
        result_pair ret(-1, -1);
        return ret;
    }
}

result_pair mapper_start_rpc(map_reduce *mr, char* inpath, char* outpath, int id, std::string address) {
    MapperClient client(
        grpc::CreateChannel(
            address, 
            grpc::InsecureChannelCredentials()
        )
    );

    result_pair response;
    response = client.mapper_start(mr->application, mr->n_threads, inpath, outpath, mr->args, id);
    return response;
}
