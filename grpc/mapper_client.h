#ifndef MAPPER_CLIENT_H
#define MAPPER_CLIENT_H

#include <inttypes.h>
#include <grpcpp/grpcpp.h>
#include "../mapper.grpc.pb.h"
#include "../tools/data.h"

using mapper::Mapper;
using mapper::MapperRequest;
using mapper::MapperReply;

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;


class MapperClient {
    public:
    MapperClient(std::shared_ptr<Channel> channel) : stub_(Mapper::NewStub(channel)) {}
    result_pair mapper_start(char* application, int32_t n_threads, char* in, char* out, char* args, int32_t id);
    private:
        std::unique_ptr<Mapper::Stub> stub_;
};

result_pair mapper_start_rpc(map_reduce *mr, char* inpath, char* outpath, int id, std::string address);

#endif