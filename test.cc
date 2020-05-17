#include <iostream>
#include "./mapreduce/mapreduce.h"

int32_t map(struct map_reduce *mr, struct kvpair *kv) {
    return 0;
}

int32_t reduce(struct map_reduce *mr, struct kvpair *kvset, size_t num) {
    return 0;
}

int main(void) {
    struct map_reduce* mr = mr_init("wordc", 1, "./test/mr_wordc/Testcase1/test1.txt", "test", " .,:;");
    mr_start(mr, 1);
    mr_finish(mr);
    mr_destroy(mr);

    return 0;
}