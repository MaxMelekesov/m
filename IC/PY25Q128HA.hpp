#pragma once
#include <IIO_Sync.hpp>
#include <IMemory.hpp>
#include <IPin.hpp>
#include <ITime.hpp>
#include <Ms.hpp>
#include <Timeout.hpp>

#include <cstdint>
#include <cstring>
#include <optional>

namespace m::ic {
template <typename TimeUnit>
class PY25Q128HA : public m::ifc::IMemory {
 public:
  using type = TimeUnit;

  PY25Q128HA(m::ifc::IIO_Sync<Ms<type>>& spi, m::ifc::mcu::IPin& cs_pin,
             m::ifc::ITime<Ms<type>>& time)
      : spi_(spi), cs_pin_(cs_pin), time_(time), timeout_(time_) {}

  std::size_t size() override { return uint32_t{16 * 1024 * 1024}; }

  bool read(std::size_t addr, std::span<uint8_t> data) override {
    std::array<uint8_t, 4> cmd;
    cmd[0] = static_cast<uint8_t>(Commands::Read);
    cmd[1] = addr >> 16;
    cmd[2] = addr >> 8;
    cmd[3] = addr;

    cs_pin_.write(1);
    auto wr_ok =
        spi_.write(cmd, Ms<type>{cmd.size() * 1'000 / spi_.getBaudrate() + 10});
    auto rd_ok = spi_.read(
        data, Ms<type>{data.size() * 1'000 / spi_.getBaudrate() + 10});
    cs_pin_.write(0);

    return wr_ok && rd_ok;
  }

  bool write(std::size_t addr, std::span<uint8_t const> data) override {
    // Write partial 4K sector
    if (auto remaining_bytes = 0x10'00 - (addr & 0xF'FF);
        remaining_bytes != 0x10'00) {
      if (remaining_bytes > data.size()) remaining_bytes = data.size();

      std::array<uint8_t, 4'096> temp_buf;
      if (!read(addr & ~0xF'FF, temp_buf)) return false;
      memcpy(temp_buf.data() + (addr % 4'096), data.data(), remaining_bytes);
      if (!erase_4K(addr)) return false;
      if (!writeSector_4K(addr & ~0xF'FF, temp_buf)) return false;

      addr += remaining_bytes;
      data = data.subspan(remaining_bytes);
    }

    // Write full 4K sectors
    while (data.size() >= 4'096) {
      if (!erase_4K(addr)) return false;
      if (!writeSector_4K(addr, data.first(4'096))) return false;

      addr += 4'096;
      data = data.subspan(4'096);
    }

    // Write partial 4K sector
    if (data.size() != 0) {
      std::array<uint8_t, 4'096> temp_buf;
      if (!read(addr, temp_buf)) return false;
      memcpy(temp_buf.data() + (addr % 4'096), data.data(), data.size());
      if (!erase_4K(addr)) return false;
      if (!writeSector_4K(addr, temp_buf)) return false;
    }

    return true;
  }

 private:
  m::ifc::IIO_Sync<Ms<type>>& spi_;
  m::ifc::mcu::IPin& cs_pin_;
  m::ifc::ITime<Ms<type>>& time_;
  m::Timeout<Ms<type>> timeout_;

  enum class Commands : uint8_t {
    Page_Prog = 0x02,
    Read = 0x03,
    Write_Disable = 0x04,
    Read_Status = 0x05,
    Write_Enable = 0x06,
    Erase_4K = 0x20,
    Read_Status_1 = 0x35,

  };

  bool setWriteMode(bool value) {
    std::array<uint8_t, 1> cmd;
    if (value) {
      cmd[0] = static_cast<uint8_t>(Commands::Write_Enable);
    } else {
      cmd[0] = static_cast<uint8_t>(Commands::Write_Disable);
    }

    cs_pin_.write(1);
    auto wr_ok =
        spi_.write(cmd, Ms<type>{cmd.size() * 1'000 / spi_.getBaudrate() + 10});
    cs_pin_.write(0);

    return wr_ok;
  }

  // Addr must be start of 4K sector, 0xXX'XX'X0'00
  bool writeSector_4K(uint32_t addr, std::span<uint8_t const> data) {
    for (uint32_t i = 0; i < 4'096 / 256; ++i) {
      if (!writeBlock(addr, data.first(256))) return false;
      addr += 256;
      data = data.subspan(256);
    }

    return true;
  }

  // Max size - 256 bytes
  // Block must be start of 256 byte page, 0xXX'XX'XX'00
  bool writeBlock(uint32_t addr, std::span<uint8_t const> data) {
    if (!setWriteMode(1)) return false;

    std::array<uint8_t, 4> cmd;
    cmd[0] = static_cast<uint8_t>(Commands::Page_Prog);
    cmd[1] = addr >> 16;
    cmd[2] = addr >> 8;
    cmd[3] = addr;

    cs_pin_.write(1);
    auto wr_ok =
        spi_.write(cmd, Ms<type>{cmd.size() * 1'000 / spi_.getBaudrate() + 10});
    auto wrd_ok = spi_.write(
        data, Ms<type>{data.size() * 1'000 / spi_.getBaudrate() + 10});
    cs_pin_.write(0);

    auto stat_ok = timeout_.execWithTimeout(
        [&]() -> bool {
          auto res = writeInProgress();
          return !res.value_or(false);
        },
        Ms<type>{5});

    return wr_ok && wrd_ok && stat_ok;
  }

  std::optional<bool> writeInProgress() {
    std::array<uint8_t, 1> cmd{static_cast<uint8_t>(Commands::Read_Status)};
    std::array<uint8_t, 1> resp{0};

    cs_pin_.write(1);
    auto rd_ok =
        spi_.write(cmd, Ms<type>{cmd.size() * 1'000 / spi_.getBaudrate() + 10});
    auto wr_ok = spi_.read(
        resp, Ms<type>{resp.size() * 1'000 / spi_.getBaudrate() + 10});
    cs_pin_.write(0);

    return (rd_ok && wr_ok) ? std::optional<bool>(resp[0] & uint8_t(0x01))
                            : std::nullopt;
  }

  bool erase_4K(uint32_t addr) {
    if (!setWriteMode(1)) return false;

    std::array<uint8_t, 4> cmd;
    cmd[0] = static_cast<uint8_t>(Commands::Erase_4K);
    cmd[1] = addr >> 16;
    cmd[2] = addr >> 8;
    cmd[3] = addr;

    cs_pin_.write(1);
    auto wr_ok =
        spi_.write(cmd, Ms<type>{cmd.size() * 1'000 / spi_.getBaudrate() + 10});
    cs_pin_.write(0);

    auto stat_ok = timeout_.execWithTimeout(
        [&]() -> bool {
          auto res = writeInProgress();
          return !res.value_or(false);
        },
        Ms<type>{1'000});

    return wr_ok && stat_ok;
  }
};
}  // namespace m::ic