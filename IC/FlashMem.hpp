/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef FLASHMEM_HPP
#define FLASHMEM_HPP

#include <CoroutineTask.hpp>
#include <FinalAction.hpp>
#include <IFlashMemoryAsync.hpp>
#include <IIO_Async.hpp>
#include <IIO_Sync.hpp>
#include <IPin.hpp>
#include <ITime.hpp>
#include <Ic.hpp>
#include <IoSyncAdpter.hpp>
#include <Ms.hpp>
#include <Reg.hpp>
#include <Timer.hpp>
#include <Us.hpp>
#include <concepts>
#include <coroutine>
#include <cstdint>
#include <optional>
#include <span>

namespace m::ic {

struct FlashMemRegmap {
  struct PageProgram {
    struct Addr : public m::BitField<Addr, 24> {};
    m::Reg<uint32_t, Addr, m::UnusedField<8>> value;
  };
  struct Read {
    struct Addr : public m::BitField<Addr, 24> {};
    m::Reg<uint32_t, Addr, m::UnusedField<8>> value;
  };
  struct Status {
    struct Busy : public m::BitField<Busy, 1> {};
    struct Wen : public m::BitField<Wen, 1> {};
    m::Reg<uint8_t, Busy, Wen, m::UnusedField<6>> value;
  };
  struct WriteEnable {
    m::Reg<uint8_t, m::UnusedField<8>> value;
  };
  struct EraseSector {
    struct Addr : public m::BitField<Addr, 24> {};
    m::Reg<uint32_t, Addr, m::UnusedField<8>> value;
  };
  struct EraseChip {
    m::Reg<uint8_t, m::UnusedField<8>> value;
  };

  using Regs = std::tuple<PageProgram, Read, Status, WriteEnable, EraseSector,
                          EraseChip>;
  using Map =
      m::StaticMap<uint8_t, m::Pair<PageProgram, 0x02>, m::Pair<Read, 0x03>,
                   m::Pair<Status, 0x05>, m::Pair<WriteEnable, 0x06>,
                   m::Pair<EraseSector, 0x20>, m::Pair<EraseChip, 0x60>>;
};
static_assert(m::ic::CIcInfo<FlashMemRegmap>,
              "FlashMemRegmap must satisfy CIcInfo concept");

template <m::ifc::CIO_Sync Io>
class FlashMemIc : public Ic<FlashMemIc<Io>, FlashMemRegmap> {
 public:
  FlashMemIc(Io& io) : io_(io) {}

 private:
  Io& io_;

  template <typename Reg>
  bool writeImpl(Reg reg) {
    return false;
  }

  template <typename Reg>
    requires std::same_as<Reg, FlashMemRegmap::WriteEnable> ||
             std::same_as<Reg, FlashMemRegmap::EraseChip>
  bool writeImpl(Reg reg) {
    std::array<uint8_t, 1> cmd;
    cmd[0] = static_cast<uint8_t>(FlashMemRegmap::Map::value<Reg>());

    auto wr_ok = io_.write(cmd);

    return wr_ok;
  }

  template <typename Reg>
    requires std::same_as<Reg, FlashMemRegmap::Read> ||
             std::same_as<Reg, FlashMemRegmap::PageProgram> ||
             std::same_as<Reg, FlashMemRegmap::EraseSector>
  bool writeImpl(Reg reg) {
    std::array<uint8_t, 4> cmd;
    cmd[0] = static_cast<uint8_t>(FlashMemRegmap::Map::value<Reg>());
    auto addr = reg.value.template get<typename Reg::Addr>();
    cmd[1] = addr >> 16;
    cmd[2] = addr >> 8;
    cmd[3] = addr;

    auto wr_ok = io_.write(cmd);

    return wr_ok;
  }

  template <typename Reg>
  std::optional<Reg> readImpl() {
    return std::nullopt;
  }

  std::optional<FlashMemRegmap::Status> readImpl() {
    std::array<uint8_t, 1> cmd{static_cast<uint8_t>(
        FlashMemRegmap::Map::value<FlashMemRegmap::Status>())};
    std::array<uint8_t, 1> resp{0};

    auto rd_ok = io_.write(cmd);
    auto wr_ok = io_.read(resp);

    FlashMemRegmap::Status status{resp[0]};

    return (rd_ok && wr_ok) ? status : std::nullopt;
  }

  friend class Ic<FlashMemIc<Io>, FlashMemRegmap>;
};

template <m::ifc::CIO_Async Io, m::ifc::CMs MsT, m::ifc::CTime<MsT> TimeT>
class FlashMemAsync : public m::ifc::IFlashMemoryAsync {
 public:
  FlashMemAsync(Io& io, m::ifc::mcu::IPin& cs, TimeT& time)
      : io_(io),
        cs_(cs),
        time_(time),
        sync_io_(io_, time_,
                 [](auto baud, std::size_t size) {
                   return MsT{size * 1'000 / baud.value() + 4};
                 }),
        ic_(sync_io_) {}

  void handle() {
    erase_task_.resume();
    read_task_.resume();
    write_task_.resume();
  }

  std::size_t sectorSize() override { return 4'096; }
  std::size_t sectorCount() override {
    return 1 * 1'024 * 1'024 / sectorSize();
  }

  bool startErase(std::size_t addr, std::size_t sectors) override {
    if (!eraseDone() || !readDone() || !writeDone()) return false;
    auto start_sector = addr / sectorSize();
    if (start_sector + sectors > sectorCount()) return false;

    //  TODO: erase chip;

    erase_task_ = erase_impl(addr, sectors, sectorSize());
    return true;
  }
  bool eraseDone() override { return erase_task_.done(); }

  bool startWrite(std::size_t addr, std::span<uint8_t const> data) override {
    if (!eraseDone() || !readDone() || !writeDone()) return false;
    if (addr + data.size() > sectorCount() * sectorSize()) return false;

    write_task_ = write_impl(addr, data);
    return true;
  }
  bool writeDone() override { return write_task_.done(); }

  bool startRead(std::size_t addr, std::span<uint8_t> data) override {
    if (!eraseDone() || !readDone() || !writeDone()) return false;
    if (addr + data.size() > sectorCount() * sectorSize()) return false;
    read_task_ = read_impl(addr, data);
    return true;
  }
  bool readDone() override { return read_task_.done(); }

  bool error() override { return erase_task_.result() != Result::Success; }

 private:
  Io& io_;
  m::ifc::mcu::IPin& cs_;
  TimeT& time_;

  using SyncIo =
      m::ifc::IoSyncAdapter<decltype(std::declval<Io>().getBaudrate()), MsT,
                            TimeT>;
  SyncIo sync_io_;
  m::ic::FlashMemIc<SyncIo> ic_;

  enum class Result {
    Running = 0,
    Success,
    WriteError,
    ReadError,
    BusyTimeout,
    IOError
  };

  struct EraseCoroutine : public m::CoroutineTask<EraseCoroutine, Result> {
    using CoroutineTask<EraseCoroutine, Result>::CoroutineTask;
  };
  EraseCoroutine erase_task_{nullptr};

  struct ReadCoroutine : public m::CoroutineTask<ReadCoroutine, Result> {
    using CoroutineTask<ReadCoroutine, Result>::CoroutineTask;
  };
  ReadCoroutine read_task_{nullptr};

  struct WriteCoroutine : public m::CoroutineTask<WriteCoroutine, Result> {
    using CoroutineTask<WriteCoroutine, Result>::CoroutineTask;
  };
  WriteCoroutine write_task_{nullptr};

  ReadCoroutine read_impl(std::size_t addr, std::span<uint8_t> data) {
    FlashMemRegmap::Read read;
    read.value.set<FlashMemRegmap::Read::Addr>(addr);

    auto cleanup = m::finally([&] { cs_.write(0); });

    cs_.write(1);
    if (!ic_.write(read)) {
      co_return Result::WriteError;
    }

    if (io_.readAsync(data)) {
      co_return Result::IOError;
    }

    Timer timer{time_};
    timer.restart(MsT{data.size() * 1'000 / io_.getBaudrate().value() + 2});

    while (!timer.timeOver()) {
      if (io_.readDone()) {
        co_return Result::Success;
      }
      co_await std::suspend_always{};
    }

    co_return Result::BusyTimeout;
  }

  EraseCoroutine erase_impl(std::size_t addr, std::size_t sectors,
                            std::size_t sector_size) {
    auto cleanup = m::finally([&] { cs_.write(0); });
    for (std::size_t i = 0; i < sectors; ++i) {
      cs_.write(1);
      if (!ic_.write(FlashMemRegmap::WriteEnable{})) {
        co_return Result::WriteError;
      }
      cs_.write(0);

      FlashMemRegmap::EraseSector es;
      es.value.set<FlashMemRegmap::EraseSector::Addr>(addr);
      addr += sector_size;

      cs_.write(1);
      if (!ic_.write(es)) {
        co_return Result::WriteError;
      }
      cs_.write(0);

      Timer timer{time_};
      timer.restart(MsT{301});

      while (!timer.timeOver()) {
        cs_.write(1);
        auto status = ic_.template read<FlashMemRegmap::Status>();
        if (!status) {
          co_return Result::ReadError;
        }
        cs_.write(0);
        if (!status.value()
                 .value.template get<FlashMemRegmap::Status::Busy>()) {
          break;
        }
        co_await std::suspend_always{};
      }

      if (timer.timeOver()) {
        co_return Result::BusyTimeout;
      }
    }
    co_return Result::Success;
  }

  WriteCoroutine write_impl(std::size_t addr, std::span<uint8_t const> data) {

    
  }
};
}  // namespace m::ic

#endif  // FLASHMEM_HPP