### Installation:
macOS:
```
suo xcode-select --install
brew install autoconf automake libtool shtool
git clone -b $(curl -L https://grpc.io/release) https://github.com/grpc/grpc
cd grpc
git submodule update --init
sudo make install
cd third_party/protobuf/
sudo make install
```

LINUX:
```
sudo apt-get install build-essential autoconf libtool pkg-config
git clone -b $(curl -L https://grpc.io/release) https://github.com/grpc/grpc
cd grpc
git submodule update --init
sudo make install
cd third_party/protobuf/
sudo make install
```