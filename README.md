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
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)
```

## Dependencies

- [dynarmic](https://github.com/merryhime/dynarmic)
- [efl::Poly](https://github.com/8ightfold/poly-standalone)