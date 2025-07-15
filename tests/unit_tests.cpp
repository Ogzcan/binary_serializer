#include "../include/binary_serializer/binary_serializer.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <sstream>
#include <numeric>
#include <chrono>

using namespace binary_serializer;

void test_array_types(class test_runner &runner);
void test_basic_types(class test_runner &runner);
void test_endianness(class test_runner &runner);
void test_chained_operations(class test_runner &runner);
void test_limits(class test_runner &runner);
void test_error_handling(class test_runner &runner);
void test_buffer_operations(class test_runner &runner);
void test_performance(class test_runner &runner);

class test_runner
{
private:
  int passed = 0;
  int failed = 0;
  std::string current_test;

public:
  test_runner() : passed(0), failed(0) 
  {
    std::cout << "Starting tests..." << std::endl;
    test_array_types(*this);
    test_basic_types(*this);
    test_endianness(*this);
    test_chained_operations(*this);
    test_limits(*this);
    test_error_handling(*this);
    test_buffer_operations(*this);
    test_performance(*this);
    std::cout << "Tests completed." << std::endl;

  }
  ~test_runner()
  {
    print_results();
  }
  void start_test(const std::string &name)
  {
    current_test = name;
    std::cout << "Testing " << name << "... ";
  }

  void check(bool condition, const std::string &message = "")
  {
    if (!condition)
    {
      std::cout << "FAILED" << std::endl;
      std::cout << "  " << current_test << ": " << message << std::endl;
      failed++;
    }
    else
    {
      std::cout << "PASSED" << std::endl;
      passed++;
    }
  }

  template <typename T>
  void assert_equal(const T &expected, const T &actual,
                   const std::string &message = "")
  {
    if (expected != actual)
    {
      std::cout << "FAILED" << std::endl;
      std::cout << "  " << current_test << ": " << message << std::endl;
      std::cout << "  Expected: " << expected << ", Got: " << actual
                << std::endl;
      failed++;
    }
    else
    {
      std::cout << "PASSED" << std::endl;
      passed++;
    }
  }

  void print_results()
  {
    std::cout << "\n=== Test Results ===" << std::endl;
    std::cout << "Passed: " << passed << std::endl;
    std::cout << "Failed: " << failed << std::endl;
    std::cout << "Total: " << (passed + failed) << std::endl;
    std::cout << "Success rate: " << (100.0 * passed / (passed + failed)) << "%"
              << std::endl;
  }
};

void test_basic_types(test_runner &runner)
{
  runner.start_test("int8_t serialization");
  auto data = serialize<int8_t>(-42);
  auto result = deserialize<int8_t>(data);
  runner.assert_equal<int8_t>(-42, result);

  runner.start_test("uint16_t serialization");
  data = serialize<uint16_t>(65535);
  auto result16 = deserialize<uint16_t>(data);
  runner.assert_equal<uint16_t>(65535, result16);

  runner.start_test("int32_t serialization");
  data = serialize<int32_t>(-1234567);
  auto result32 = deserialize<int32_t>(data);
  runner.assert_equal<int32_t>(-1234567, result32);

  runner.start_test("uint64_t serialization");
  data = serialize<uint64_t>(18446744073709551615ULL);
  auto result64 = deserialize<uint64_t>(data);
  runner.assert_equal<uint64_t>(18446744073709551615ULL, result64);

  runner.start_test("float serialization");
  data = serialize<float>(3.14159f);
  auto result_float = deserialize<float>(data);
  runner.check(std::abs(result_float - 3.14159f) < 1e-6,
                "Float precision issue");

  runner.start_test("double serialization");
  data = serialize<double>(2.718281828459045);
  auto result_double = deserialize<double>(data);
  runner.check(std::abs(result_double - 2.718281828459045) < 1e-15,
                "Double precision issue");

  runner.start_test("bool serialization");
  data = serialize<bool>(true);
  auto result_bool = deserialize<bool>(data);
  runner.assert_equal<bool>(true, result_bool);
}

void test_string_types(test_runner &runner)
{
  runner.start_test("string serialization");
  std::string original = "Hello, World!";
  auto data = serialize(original);
  auto result = deserialize<std::string>(data);
  runner.assert_equal(original, result);

  runner.start_test("empty string serialization");
  std::string empty = "";
  data = serialize(empty);
  result = deserialize<std::string>(data);
  runner.assert_equal(empty, result);

  runner.start_test("unicode string serialization");
  std::string unicode = "Hello 世界 🌍";
  data = serialize(unicode);
  result = deserialize<std::string>(data);
  runner.assert_equal(unicode, result);
}

void test_array_types(test_runner &runner)
{
  runner.start_test("vector<int> serialization");
  std::vector<int> vec = {1, 2, 3, 4, 5};
  auto data = serialize(vec);
  auto result = deserialize<std::vector<int>>(data);
  runner.assert_equal(vec.size(), result.size());
  runner.check(std::equal(vec.begin(), vec.end(), result.begin()),
                "Vector contents differ");

  runner.start_test("empty vector serialization");
  std::vector<int> empty;
  data = serialize(empty);
  result = deserialize<std::vector<int>>(data);
  runner.assert_equal(empty.size(), result.size());

  runner.start_test("array<float, 3> serialization");
  std::array<float, 3> arr = {1.1f, 2.2f, 3.3f};
  data = serialize(arr);
  auto result_arr = deserialize<std::array<float, 3>>(data);
  runner.check(std::equal(arr.begin(), arr.end(), result_arr.begin()),
                "Array contents differ");
}

void test_endianness(test_runner &runner)
{
  uint32_t value = 0x12345678;

  runner.start_test("little endian serialization");
  auto little_data = serialize(value, endianness::little);
  auto little_result = deserialize<uint32_t>(little_data, endianness::little);
  runner.assert_equal(value, little_result);

  runner.start_test("big endian serialization");
  auto big_data = serialize(value, endianness::big);
  auto big_result = deserialize<uint32_t>(big_data, endianness::big);
  runner.assert_equal(value, big_result);

  runner.start_test("cross-endian compatibility");

  auto cross_result = deserialize<uint32_t>(little_data, endianness::big);
  runner.assert_equal(swap_endianness(value), cross_result);
}

void test_buffer_operations(test_runner &runner)
{
  runner.start_test("buffer position tracking");
  Buffer buffer;
  buffer.write<uint32_t>(42);
  buffer.write<uint16_t>(100);

  runner.assert_equal<size_t>(6, buffer.size());

  buffer.set_position(0);
  auto val1 = buffer.read<uint32_t>();
  auto val2 = buffer.read<uint16_t>();

  runner.assert_equal<uint32_t>(42, val1);
  runner.assert_equal<uint16_t>(100, val2);
  runner.assert_equal<size_t>(6, buffer.position());

  runner.start_test("buffer underflow protection");
  buffer.set_position(0);
  try
  {
    buffer.read<uint64_t>();
    runner.check(false, "Should have thrown exception");
  }
  catch (const std::runtime_error &)
  {
    runner.check(true, "Correctly threw exception on underflow");
  }
}

void test_chained_operations(test_runner &runner)
{
  runner.start_test("chained serialization");

  Serializer serializer;
  serializer << int32_t(42) << std::string("test") << float(3.14f)
             << bool(true);

  auto data = serializer.get_data();

  Deserializer deserializer(data);
  int32_t i;
  std::string s;
  float f;
  bool b;
  deserializer >> i >> s >> f >> b;

  runner.assert_equal<int32_t>(42, i);
  runner.assert_equal<std::string>("test", s);
  runner.check(std::abs(f - 3.14f) < 1e-6, "Float precision issue");
  runner.assert_equal<bool>(true, b);
}

void test_limits(test_runner &runner)
{
  runner.start_test("integer limits");

  auto min_int = std::numeric_limits<int32_t>::min();
  auto max_int = std::numeric_limits<int32_t>::max();

  auto data = serialize(min_int);
  auto result = deserialize<int32_t>(data);
  runner.assert_equal(min_int, result);

  data = serialize(max_int);
  result = deserialize<int32_t>(data);
  runner.assert_equal(max_int, result);

  runner.start_test("large string");
  std::string large_string(10000, 'A');
  data = serialize(large_string);
  auto result_string = deserialize<std::string>(data);
  runner.assert_equal(large_string, result_string);
}

void test_error_handling(test_runner &runner)
{
  runner.start_test("malformed data handling");

  std::vector<uint8_t> malformed_data = {0xFF, 0xFF, 0xFF,
                                        0xFF};

  try
  {
    Deserializer deserializer(malformed_data);
    std::string result;
    deserializer >> result;
    runner.check(false, "Should have thrown exception");
  }
  catch (const std::runtime_error &)
  {
    runner.check(true, "Correctly handled malformed data");
  }
}

void test_performance(test_runner &runner)
{
  runner.start_test("performance baseline");

  const size_t iterations = 10000;
  std::vector<int> test_data(100);
  std::iota(test_data.begin(), test_data.end(), 0);

  auto start = std::chrono::high_resolution_clock::now();

  for (size_t i = 0; i < iterations; ++i)
  {
    auto data = serialize(test_data);
    auto result = deserialize<std::vector<int>>(data);
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);

  runner.check(duration.count() < 1000000, "Performance test took too long");

  std::cout << "  Performance: " << duration.count() << " microseconds for "
            << iterations << " iterations" << std::endl;
}

int main()
{
  test_runner runner;
  return 0;
}