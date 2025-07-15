# Binary Serializer Library

A C++ header-only library for binary serialization with endianness support.

## Features
- Serialization of primitive types (integers, floats)
- String and array serialization
- Endianness conversion
- Simple API

## Usage

```cpp
#include <binary_serializer/binary_serializer.hpp>

// serialization
auto data = binary_serializer::serialize<int>(42);

// Deserialization
int value = binary_serializer::deserialize<int>(data);
```

```bash
mkdir build && cd build
cmake ..
cmake --build .
```