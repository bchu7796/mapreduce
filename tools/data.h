#ifndef DATA_H
#define DATA_H

#include <inttypes.h>
#include "../mapreduce/mapreduce.h"

struct result_pair {
    result_pair();
    result_pair(int32_t id, int32_t result);
    bool operator==(const result_pair& a);
    bool operator!=(const result_pair& a);
    int32_t id;
    int32_t result;
};

struct rpc_args {
    map_reduce *mr;
    char *inpath;
    char *outpath;
    int id;
};


#endif