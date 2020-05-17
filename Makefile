LDFLAGS = -L/usr/local/lib, `pkg-config --libs protobuf grpc++`\
          -lgrpc++_reflection\
           -ldl -lpthread\

CXX = g++
CPPFLAGS += `pkg-config --cflags protobuf grpc`
CXXFLAGS += -std=c++11 

GRPC_CPP_PLUGIN = grpc_cpp_plugin
GRPC_CPP_PLUGIN_PATH ?= `which $(GRPC_CPP_PLUGIN)`

SRC1 = $(wildcard ./mapreduce/*.cc)
SRC2 = $(wildcard ./mapper/*.cc)
SRC3 = $(wildcard ./reducer/*.cc)
SRC4 = $(wildcard ./grpc/*.cc)
SRC5 = $(wildcard ./tools/*.cc)

MAPREDUCE_OBJ = $(patsubst %.cc,%.o,$(SRC1))
MAPPER_OBJ = $(patsubst %.cc,%.o,$(SRC2))
REDUCER_OBJ = $(patsubst %.cc,%.o,$(SRC3))
GRPC_OBJ = $(patsubst %.cc,%.o,$(SRC4))
TOOLS_OBJ = $(patsubst %.cc,%.o,$(SRC5))

all: ./build/wordc_client ./build/wordc_mapper ./build/wordc_reducer ./build/grep_client ./build/grep_mapper ./build/grep_reducer

./build/wordc_client: mapper.pb.o mapper.grpc.pb.o reducer.pb.o reducer.grpc.pb.o bin/driver.o bin/mr_wordc.o $(MAPREDUCE_OBJ) $(GRPC_OBJ) $(TOOLS_OBJ) test.o
	$(CXX) $^ $(LDFLAGS) -o $@

./build/wordc_mapper: mapper.pb.o mapper.grpc.pb.o reducer.pb.o reducer.grpc.pb.o $(MAPPER_OBJ) $(MAPREDUCE_OBJ) bin/mr_wordc.o $(GRPC_OBJ) $(TOOLS_OBJ) driver.o
	$(CXX) $^ $(LDFLAGS) -o $@

./build/wordc_reducer: mapper.pb.o mapper.grpc.pb.o reducer.pb.o reducer.grpc.pb.o $(REDUCER_OBJ) $(MAPREDUCE_OBJ) bin/mr_wordc.o $(TOOLS_OBJ)
	$(CXX) $^ $(LDFLAGS) -o $@

./build/grep_client: mapper.pb.o mapper.grpc.pb.o reducer.pb.o reducer.grpc.pb.o bin/driver.o bin/mr_grep.o $(MAPREDUCE_OBJ) $(GRPC_OBJ) $(TOOLS_OBJ)
	$(CXX) $^ $(LDFLAGS) -o $@

./build/grep_mapper: mapper.pb.o mapper.grpc.pb.o reducer.pb.o reducer.grpc.pb.o $(MAPPER_OBJ) $(MAPREDUCE_OBJ) bin/mr_grep.o $(GRPC_OBJ) $(TOOLS_OBJ)
	$(CXX) $^ $(LDFLAGS) -o $@

./build/grep_reducer: mapper.pb.o mapper.grpc.pb.o reducer.pb.o reducer.grpc.pb.o $(REDUCER_OBJ) $(MAPREDUCE_OBJ) bin/mr_grep.o $(TOOLS_OBJ)
	$(CXX) $^ $(LDFLAGS) -o $@


%.grpc.pb.cc: %.proto
	protoc --grpc_out=. --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN_PATH) $<

%.pb.cc: %.proto
	protoc --cpp_out=. $<

clean:
	rm -f *.o *.pb.cc *.pb.h ./build/wordc_client ./build/wordc_mapper ./build/wordc_reducer \
	                         ./build/grep_client ./build/grep_mapper ./build/grep_reducer \
							 $(MAPREDUCE_OBJ) $(GRPC_OBJ) $(MAPPER_OBJ) $(REDUCER_OBJ) $(TOOLS_OBJ) \