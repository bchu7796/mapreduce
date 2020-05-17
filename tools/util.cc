#include "util.h"

#include <iostream>
#include <inttypes.h>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include "../mapreduce/mapreduce.h"

// this is used to split string with delimeter
void split(const std::string& s, std::vector<std::string>& parameters, const char delim) {
    parameters.clear();
    std::istringstream iss(s);
    std::string temp;

    while (std::getline(iss, temp, delim)) {
        parameters.emplace_back(std::move(temp));
    }
    return;
}

int32_t read_config(std::string config_name, map_reduce* mr) {
    std::fstream config_file;
    config_file.open(config_name, std::ios::in);
    std::string line;
    std::vector<std::string> parsed_line;
    if(!config_file) {
        return -1;
    }
    while(getline(config_file, line)) {
        split(line, parsed_line);
        if(parsed_line[0] == "MAPPER") {
            for(int32_t i = 0; i < std::stoi(parsed_line[1]); i++) {
                mr->mapper_addresses.push_back(parsed_line[2]);
            }
        }
        else if(parsed_line[0] == "REDUCER") {
            mr->reducer_address = parsed_line[1];
        }
    }
    return 0;
}

