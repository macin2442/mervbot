This is catids MERVBot source.

## Build Instructions

```
# install dependencies
sudo apt install cmake build-essential
sudo apt install libboost-program-options-dev libboost-filesystem-dev libboost-iostreams-dev

# checkout submodules
git submodule update --init --recursive

# configure and build
mkdir build
cd build
cmake ..
make
```

