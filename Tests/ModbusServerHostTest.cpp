// Host runner for the ModbusServer tests (compile with a C++23 host compiler).
//
//   g++ -std=c++23 -I m/Interfaces -I m/Interfaces/Mcu -I m/Logic/Units \
//       -I m/Logic/FlowControl -I m/Logic/Protocol \
//       m/Tests/ModbusServerHostTest.cpp -o modbus_server_host_test

#include <CoroScheduler.hpp>
#include <cstddef>

// Coroutine frame pool for the test binary (larger than the 384-byte default
// to be safe with the coroRun/coroDelay nesting).
namespace m {
template <>
struct CoroTraits<> {
  static constexpr std::size_t slot_size = 768;
  static constexpr std::size_t capacity = 8;
};
}  // namespace m

#include "ModbusServerTest.hpp"

#include <cstdio>

int main() {
  const bool ok = m::tsts::modbusServerRunAllTests();
  std::printf(ok ? "MODBUS_SERVER_TESTS: OK\n" : "MODBUS_SERVER_TESTS: FAIL\n");
  return ok ? 0 : 1;
}
