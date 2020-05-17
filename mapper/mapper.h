#ifndef MAPPER_H
#define MAPPER_H

#include <vector>
#include <string>

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


struct mapper_struct {
    int id;
    map_reduce *mr;
    int finished;
};


class MapperServiceImplementation final : public Mapper::Service {
public:
    Status mapper_start(
        ServerContext* context, 
        const MapperRequest* request, 
        MapperReply* reply
    ) override;

private:
    int32_t mapper_start_(struct map_reduce *mr, int32_t id);

    int32_t sender(mapper_struct* map_worker, int id);

    mapper_struct* mapper_init(struct map_reduce *mr, int32_t id);

    void mapper_destroy(struct mapper_struct *map_worker);

    int32_t flush_rpc(struct map_reduce *mr, int32_t id, std::string address);
};

#endif