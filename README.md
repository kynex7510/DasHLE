# DasHLE

(WIP) Run android versions of Geometry Dash through high level emulation.

## How to

### Setup

```
git clone --recurse-submodules -j$(nproc) https://github.com/kynex7510/DasHLE
```

### Compilation

```
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DDASHLE_TARGET=Linux ..
make -j16
```