# DasHLE

(WIP) Run android versions of Geometry Dash through high level emulation.

## How to

### Setup

```
git clone -j$(nproc) https://github.com/kynex7510/DasHLE
```

### Compilation

```
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DCPM_Dynarmic_SOURCE=/home/user/Documents/repos/dynarmic -DDASHLE_HOST=Linux -DDASHLE_GUESTS=ARM ..
make -j$(nproc)
```

## Dependencies

- [dynarmic](https://github.com/merryhime/dynarmic)
- [efl::Poly](https://github.com/8ightfold/poly-standalone)