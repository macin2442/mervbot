This is catids MERVBot source.

## Build Instructions

```
# install dependencies
sudo apt install libboost-program-options1.71-dev libboost-filesystem1.71-dev libboost-iostreams1.71-dev

# checkout submodules
git submodule update --init --recursive

# configure and build
mkdir build
cd build
cmake ..
make
```

