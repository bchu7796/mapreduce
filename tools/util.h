#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <vector>
#include <inttypes.h>
#include "../mapreduce/mapreduce.h"

// this is used to split string with delimeter
void split(const std::string& s, std::vector<std::string>& parameters, const char delim = ' ');
int32_t read_config(std::string config_name, map_reduce* mr);

#endif