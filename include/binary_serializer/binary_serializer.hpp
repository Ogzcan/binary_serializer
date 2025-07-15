#pragma once

#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace binary_serializer
{

enum class endianness
{
  little,
  big,
  native
};

inline endianness get_system_endianness()
{
  constexpr uint32_t test = 0x12345678;
  return (*reinterpret_cast<const uint8_t *>(&test) == 0x78)
             ? endianness::little
             : endianness::big;
}

template <typename T> inline T swap_endianness(T value)
{
  static_assert(std::is_arithmetic_v<T>, "Type must be arithmetic");

  if constexpr (sizeof(T) == 1)
  {
    return value;
  }
  else if constexpr (sizeof(T) == 2)
  {
    using U = std::conditional_t<std::is_floating_point_v<T>, uint16_t, T>;
    U temp;
    std::memcpy(&temp, &value, sizeof(T));
    temp = ((temp & 0xFF) << 8) | ((temp >> 8) & 0xFF);
    T result;
    std::memcpy(&result, &temp, sizeof(T));
    return result;
  }
  else if constexpr (sizeof(T) == 4)
  {
    using U = std::conditional_t<std::is_floating_point_v<T>, uint32_t, T>;
    U temp;
    std::memcpy(&temp, &value, sizeof(T));
    temp = ((temp & 0xFF) << 24) | ((temp & 0xFF00) << 8) |
           ((temp >> 8) & 0xFF00) | ((temp >> 24) & 0xFF);
    T result;
    std::memcpy(&result, &temp, sizeof(T));
    return result;
  }
  else if constexpr (sizeof(T) == 8)
  {
    using U = std::conditional_t<std::is_floating_point_v<T>, uint64_t, T>;
    U temp;
    std::memcpy(&temp, &value, sizeof(T));
    temp = ((temp & 0xFF) << 56) | ((temp & 0xFF00) << 40) |
           ((temp & 0xFF0000) << 24) | ((temp & 0xFF000000) << 8) |
           ((temp >> 8) & 0xFF000000) | ((temp >> 24) & 0xFF0000) |
           ((temp >> 40) & 0xFF00) | ((temp >> 56) & 0xFF);
    T result;
    std::memcpy(&result, &temp, sizeof(T));
    return result;
  }
}

class Buffer
{
private:
  std::vector<uint8_t> m_data;
  size_t m_position = 0;
  endianness m_endianness;

public:
  explicit Buffer(endianness endian = endianness::native) : m_endianness(endian)
  {
    if (m_endianness == endianness::native)
      m_endianness = get_system_endianness();
  }

  explicit Buffer(std::vector<uint8_t> data,
                  endianness endian = endianness::native)
      : m_data(std::move(data)), m_endianness(endian)
  {
    if (m_endianness == endianness::native)
    {
      m_endianness = get_system_endianness();
    }
  }

  void reserve(size_t size)
  {
    m_data.reserve(size);
  }
  void clear()
  {
    m_data.clear();
    m_position = 0;
  }

  size_t size() const
  {
    return m_data.size();
  }
  size_t position() const
  {
    return m_position;
  }
  void set_position(size_t pos)
  {
    m_position = pos;
  }

  const uint8_t *data() const
  {
    return m_data.data();
  }
  const std::vector<uint8_t> &vector() const
  {
    return m_data;
  }

  endianness get_endianness() const
  {
    return m_endianness;
  }
  void set_endianness(endianness endian)
  {
    m_endianness = endian;
  }

  template <typename T> void write_raw(const T &value)
  {
    const auto *bytes = reinterpret_cast<const uint8_t *>(&value);
    m_data.insert(m_data.end(), bytes, bytes + sizeof(T));
  }

  template <typename T> T read_raw()
  {
    if (m_position + sizeof(T) > m_data.size())
    {
      throw std::runtime_error("Buffer underflow");
    }

    T value;
    std::memcpy(&value, m_data.data() + m_position, sizeof(T));
    m_position += sizeof(T);
    return value;
  }

  template <typename T> void write(T value)
  {
    static_assert(std::is_arithmetic_v<T>, "Type must be arithmetic");

    if (m_endianness != get_system_endianness())
    {
      value = swap_endianness(value);
    }
    write_raw(value);
  }

  template <typename T> T read()
  {
    static_assert(std::is_arithmetic_v<T>, "Type must be arithmetic");

    T value = read_raw<T>();
    if (m_endianness != get_system_endianness())
    {
      value = swap_endianness(value);
    }
    return value;
  }

  void write_string(const std::string &str)
  {
    write<uint32_t>(static_cast<uint32_t>(str.length()));
    m_data.insert(m_data.end(), str.begin(), str.end());
  }

  std::string read_string()
  {
    auto length = read<uint32_t>();
    if (m_position + length > m_data.size())
    {
      throw std::runtime_error("String extends beyond buffer");
    }

    std::string result(m_data.begin() + m_position,
                       m_data.begin() + m_position + length);
    m_position += length;
    return result;
  }

  template <typename T> void write_array(const T *array, size_t count)
  {
    write<uint32_t>(static_cast<uint32_t>(count));
    for (size_t i = 0; i < count; ++i)
    {
      write(array[i]);
    }
  }

  template <typename T> std::vector<T> read_array()
  {
    auto count = read<uint32_t>();
    std::vector<T> result;
    result.reserve(count);

    for (uint32_t i = 0; i < count; ++i)
    {
      result.push_back(read<T>());
    }
    return result;
  }
};

class Serializer
{
private:
  Buffer m_buffer;

public:
  explicit Serializer(endianness endian = endianness::native) : m_buffer(endian){}

  // Primitive types
  template <typename T> Serializer &operator<<(T value)
  {
    static_assert(std::is_arithmetic_v<T>, "Type must be arithmetic");
    m_buffer.write(value);
    return *this;
  }

  Serializer &operator<<(const std::string &str)
  {
    m_buffer.write_string(str);
    return *this;
  }

  Serializer &operator<<(const char *str)
  {
    m_buffer.write_string(std::string(str));
    return *this;
  }

  template <typename T, size_t N>
  Serializer &operator<<(const std::array<T, N> &arr)
  {
    m_buffer.write_array(arr.data(), N);
    return *this;
  }

  template <typename T> Serializer &operator<<(const std::vector<T> &vec)
  {
    m_buffer.write_array(vec.data(), vec.size());
    return *this;
  }

  const Buffer &get_buffer() const
  {
    return m_buffer;
  }
  std::vector<uint8_t> get_data() const
  {
    return m_buffer.vector();
  }
  void clear()
  {
    m_buffer.clear();
  }
};

class Deserializer
{
private:
  Buffer m_buffer;

public:
  explicit Deserializer(std::vector<uint8_t> data,
                        endianness endian = endianness::native)
      : m_buffer(std::move(data), endian)
  {}

  // Primitive types
  template <typename T> Deserializer &operator>>(T &value)
  {
    static_assert(std::is_arithmetic_v<T>, "Type must be arithmetic");
    value = m_buffer.read<T>();
    return *this;
  }

  Deserializer &operator>>(std::string &str)
  {
    str = m_buffer.read_string();
    return *this;
  }

  template <typename T, size_t N>
  Deserializer &operator>>(std::array<T, N> &arr)
  {
    auto vec = m_buffer.read_array<T>();
    if (vec.size() != N)
    {
      throw std::runtime_error("Array size mismatch");
    }
    std::copy(vec.begin(), vec.end(), arr.begin());
    return *this;
  }

  template <typename T> Deserializer &operator>>(std::vector<T> &vec)
  {
    vec = m_buffer.read_array<T>();
    return *this;
  }

  bool has_more() const
  {
    return m_buffer.position() < m_buffer.size();
  }
  size_t remaining() const
  {
    return m_buffer.size() - m_buffer.position();
  }
};

template <typename T>
std::vector<uint8_t> serialize(const T &value,
                               endianness endian = endianness::native)
{
  Serializer serializer(endian);
  serializer << value;
  return serializer.get_data();
}

template <typename T>
T deserialize(const std::vector<uint8_t> &data,
              endianness endian = endianness::native)
{
  Deserializer deserializer(data, endian);
  T value;
  deserializer >> value;
  return value;
}

}