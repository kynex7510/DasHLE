# ABI

WinAPI convention (`WORD` = 16, `DWORD` = 32, `QWORD` = 64)

## ARM

### Type classes

Signed/unsigned is irrelevant.

- `Integer`:  This class consists of integral types that into one of the general purpose registers + u64.
- `Floating point`: half, single, double
- `Containerized vector`: ???
- `Pointer`

### Result return

- A half float is returned in the least significant 16 bits of r0.
- - Other bits?
- A fundamental is returned in r0.
- - If it's smaller than 4 bytes it's zero- or sign-extended.
- A 64-bit is returned in r0 and r1.
- A 128-bit is returned in r0-r3.
- A composite that fits in r0 goes in r0.
- - OOB bits are unspecified.
- Other composites are stored in memory (ptr passed as an argument).

## Linux x64

### Parameter classes

- `INTEGER`: This class consists of integral types that fit into one of the general purpose registers.
- `SSE`: The class consists of types that fit into a vector register.
- `SSEUP`: The class consists of types that fit into a vector register and can be passed and returned in the upper bytes of it.
- `X87`, `X87UP`: These classes consists of types that will be returned via the x87 FPU.
- `COMPLEX_X87`: This class consists of types that will be returned via the x87 FPU.
- `NO_CLASS`: This class is used as initializer in the algorithms. It will be used for padding and empty structures and unions.
- `MEMORY`: This class consists of types that will be passed and returned in memory via the stack.

### Argument types

`INTEGER`: `_Bool`, `bool`, `char`, `short`, `int`, `long`, `long long`, `T*`
`SSE`: `float`, `double`, `_Decimal32`, `_Decimal64`, `__m64`

Additionally:

- - `__float128`, `_Decimal128`, `__m128` are split into two QWORDs. The least significant ones belong to class `SSE`, the most significant one to class `SSEUP`.

- - `__m256` is split into four QWORD chunks. The least significant one belongs to class `SSE` and all the others to class `SSEUP`.

- `__int128` is treated as if it were implemented as:

```c
typedef struct {
long low, high;
} __int128;
```

with the exception that arguments of type `__int128` that are stored in memory must be aligned on a 16-byte boundary.

- `complex T` where T is one of the types float or double is treated as if they are implemented as:

```c
struct complex T {
    T real;
    T imag;
};
```

- A variable of type complex long double is classified as type `COMPLEX_X87`.

- If the size of an object is larger than four QWORDs, or it contains unaligned fields, it has class `MEMORY`.
- If a C++ object has either a non-trivial copy constructor or a non-trivial destructor, it is passed as a pointer that has class `INTEGER`.
- If the size of the aggregate exceeds a single QWORD, each is classified separately. Each QWORD gets initialized to class `NO_CLASS`.

- Each field of an object is classified recursively so that always two fields are considered. The resulting class is calculated according to the classes of the fields in the QWORD:
- - If both classes are equal, this is the resulting class.
- - If one of the classes is `NO_CLASS`, the resulting class is the other class.
- - If one of the classes is `MEMORY`, the result is the `MEMORY` class.
- - If one of the classes is `INTEGER`, the result is the `INTEGER`.
- - If one of the classes is `X87`, `X87UP`, `COMPLEX_X87` class, `MEMORY` is used as class.
- - Otherwise class SSE is used.

- Then a post merger cleanup is done:
- - If one of the classes is `MEMORY`, the whole argument is passed in memory.
- - If `X87UP` is not preceded by `X87`, the whole argument is passed in memory.
- - If the size of the aggregate exceeds two QWORDs and the first QWORD isn’t `SSE` or any other QWORD isn’t `SSEUP`, the whole argument is passed in memory.
- - If `SSEUP` is not preceded by `SSE` or `SSEUP`, it is converted to `SSE`.

### Passing

- `INTEGER`: the next available register of the sequence `rdi`, `rsi`, `rdx`, `rcx`, `r8` and `r9` is used.
- - When a value of type `_Bool`, `bool` is returned or passed in a register or on the stack, bit 0 contains the truth value and bits 1 to 7 shall be zero.
- - If there are no registers available for any QWORD of an argument, the whole argument is passed on the stack.
- - If registers have already been assigned for some
QWORDs of such an argument, the assignments get reverted.
- - Once registers are assigned, the arguments passed in memory are pushed on the stack in reversed (right-to-left) order.

- `MEMORY`: pass the argument on the stack.
  
- `SSE`: the next available vector register is used, the registers are taken in the order from `xmm0` to `xmm7`.
- - Additional arguments are pushed on the stack.

- `SSEUP`: the QWORD is passed in the next available QWORD chunk of the last used vector register.

- `X87`, `X87UP`, `COMPLEX_X87`: passed in memory.

## Windows x64

