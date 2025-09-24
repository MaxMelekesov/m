/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */
#ifndef FDC1004_HPP
#define FDC1004_HPP

#include <DebugLogger.hpp>
#include <Fsm_v4.hpp>
#include <IIO_Async.hpp>
#include <ITime.hpp>
#include <IcSync.hpp>
#include <Reg.hpp>
#include <StaticMap.hpp>
#include <Timeout.hpp>
#include <Us.hpp>
#include <cstdint>

namespace m::ic {

struct Fdc1004 {
  struct Meas1Msb {
    struct Msb : public m::BitField<Msb, 16> {};

    m::Reg<uint16_t, Msb> value;
  };
  struct Meas1Lsb {
    struct Lsb : public m::BitField<Lsb, 8> {};

    m::Reg<uint16_t, m::UnusedField<8>, Lsb> value;
  };

  struct Meas2Msb {
    struct Msb : public m::BitField<Msb, 16> {};

    m::Reg<uint16_t, Msb> value;
  };
  struct Meas2Lsb {
    struct Lsb : public m::BitField<Lsb, 8> {};

    m::Reg<uint16_t, m::UnusedField<8>, Lsb> value;
  };

  struct Meas3Msb {
    struct Msb : public m::BitField<Msb, 16> {};

    m::Reg<uint16_t, Msb> value;
  };
  struct Meas3Lsb {
    struct Lsb : public m::BitField<Lsb, 8> {};

    m::Reg<uint16_t, m::UnusedField<8>, Lsb> value;
  };

  struct Meas4Msb {
    struct Msb : public m::BitField<Msb, 16> {};

    m::Reg<uint16_t, Msb> value;
  };
  struct Meas4Lsb {
    struct Lsb : public m::BitField<Lsb, 8> {};

    m::Reg<uint16_t, m::UnusedField<8>, Lsb> value;
  };

  struct ConfMeas1 {
    struct Capdac : public m::BitField<Capdac, 5> {};
    struct Chb : public m::BitField<Chb, 3> {
      enum : uint8_t { Cin1 = 0, Cin2, Cin3, Cin4, Capdac, Disabled = 0b111 };
    };
    struct Cha : public m::BitField<Cha, 3> {
      enum : uint8_t { Cin1 = 0, Cin2, Cin3, Cin4 };
    };

    m::Reg<uint16_t, m::UnusedField<5>, Capdac, Chb, Cha> value;
  };

  struct ConfMeas2 {
    struct Capdac : public m::BitField<Capdac, 5> {};
    struct Chb : public m::BitField<Chb, 3> {
      enum : uint8_t { Cin1 = 0, Cin2, Cin3, Cin4, Capdac, Disabled = 0b111 };
    };
    struct Cha : public m::BitField<Cha, 3> {
      enum : uint8_t { Cin1 = 0, Cin2, Cin3, Cin4 };
    };

    m::Reg<uint16_t, m::UnusedField<5>, Capdac, Chb, Cha> value;
  };

  struct ConfMeas3 {
    struct Capdac : public m::BitField<Capdac, 5> {};
    struct Chb : public m::BitField<Chb, 3> {
      enum : uint8_t { Cin1 = 0, Cin2, Cin3, Cin4, Capdac, Disabled = 0b111 };
    };
    struct Cha : public m::BitField<Cha, 3> {
      enum : uint8_t { Cin1 = 0, Cin2, Cin3, Cin4 };
    };

    m::Reg<uint16_t, m::UnusedField<5>, Capdac, Chb, Cha> value;
  };

  struct ConfMeas4 {
    struct Capdac : public m::BitField<Capdac, 5> {};
    struct Chb : public m::BitField<Chb, 3> {
      enum : uint8_t { Cin1 = 0, Cin2, Cin3, Cin4, Capdac, Disabled = 0b111 };
    };
    struct Cha : public m::BitField<Cha, 3> {
      enum : uint8_t { Cin1 = 0, Cin2, Cin3, Cin4 };
    };

    m::Reg<uint16_t, m::UnusedField<5>, Capdac, Chb, Cha> value;
  };

  struct FdcConf {
    struct Done4 : public m::BitField<Done4, 1> {};
    struct Done3 : public m::BitField<Done3, 1> {};
    struct Done2 : public m::BitField<Done2, 1> {};
    struct Done1 : public m::BitField<Done1, 1> {};
    struct Meas4 : public m::BitField<Meas4, 1> {};
    struct Meas3 : public m::BitField<Meas3, 1> {};
    struct Meas2 : public m::BitField<Meas2, 1> {};
    struct Meas1 : public m::BitField<Meas1, 1> {};
    struct Repeat : public m::BitField<Repeat, 1> {};
    struct Rate : public m::BitField<Rate, 2> {
      enum : uint8_t { R_100Sps = 1, R_200Sps, R_400Sps };
    };
    struct Reset : public m::BitField<Reset, 1> {};

    m::Reg<uint16_t, Done4, Done3, Done2, Done1, Meas4, Meas3, Meas2, Meas1,
           Repeat, m::UnusedField<1>, Rate, m::UnusedField<3>, Reset>
        value;
  };

  struct OffsetCal1 {
    struct Decimal : public m::BitField<Decimal, 11> {};
    struct Integer : public m::BitField<Integer, 5> {};

    m::Reg<uint16_t, Decimal, Integer> value;
  };

  struct OffsetCal2 {
    struct Decimal : public m::BitField<Decimal, 11> {};
    struct Integer : public m::BitField<Integer, 5> {};

    m::Reg<uint16_t, Decimal, Integer> value;
  };

  struct OffsetCal3 {
    struct Decimal : public m::BitField<Decimal, 11> {};
    struct Integer : public m::BitField<Integer, 5> {};

    m::Reg<uint16_t, Decimal, Integer> value;
  };

  struct OffsetCal4 {
    struct Decimal : public m::BitField<Decimal, 11> {};
    struct Integer : public m::BitField<Integer, 5> {};

    m::Reg<uint16_t, Decimal, Integer> value;
  };

  struct GainCal1 {
    struct Decimal : public m::BitField<Decimal, 14> {};
    struct Integer : public m::BitField<Integer, 2> {};

    m::Reg<uint16_t, Decimal, Integer> value;
  };

  struct GainCal2 {
    struct Decimal : public m::BitField<Decimal, 14> {};
    struct Integer : public m::BitField<Integer, 2> {};

    m::Reg<uint16_t, Decimal, Integer> value;
  };

  struct GainCal3 {
    struct Decimal : public m::BitField<Decimal, 14> {};
    struct Integer : public m::BitField<Integer, 2> {};

    m::Reg<uint16_t, Decimal, Integer> value;
  };

  struct GainCal4 {
    struct Decimal : public m::BitField<Decimal, 14> {};
    struct Integer : public m::BitField<Integer, 2> {};

    m::Reg<uint16_t, Decimal, Integer> value;
  };

  struct Manufacturer {
    struct Id : public m::BitField<Id, 16> {};

    m::Reg<uint16_t, Id> value;
  };

  struct Device {
    struct Id : public m::BitField<Id, 16> {};

    m::Reg<uint16_t, Id> value;
  };

  using Regs =
      std::tuple<Meas1Msb, Meas1Lsb, Meas2Msb, Meas2Lsb, Meas3Msb, Meas3Lsb,
                 Meas4Msb, Meas4Lsb, ConfMeas1, ConfMeas2, ConfMeas3, ConfMeas4,
                 FdcConf, OffsetCal1, OffsetCal2, OffsetCal3, OffsetCal4,
                 GainCal1, GainCal2, GainCal3, GainCal4, Manufacturer, Device>;

  struct Map : public m::StaticMap<
                   uint8_t, m::Pair<Meas1Msb, 0x00>, m::Pair<Meas1Lsb, 0x01>,
                   m::Pair<Meas2Msb, 0x02>, m::Pair<Meas2Lsb, 0x03>,
                   m::Pair<Meas3Msb, 0x04>, m::Pair<Meas3Lsb, 0x05>,
                   m::Pair<Meas4Msb, 0x06>, m::Pair<Meas4Lsb, 0x07>,
                   m::Pair<ConfMeas1, 0x08>, m::Pair<ConfMeas2, 0x09>,
                   m::Pair<ConfMeas3, 0x0A>, m::Pair<ConfMeas4, 0x0B>,
                   m::Pair<FdcConf, 0x0C>, m::Pair<OffsetCal1, 0x0D>,
                   m::Pair<OffsetCal2, 0x0E>, m::Pair<OffsetCal3, 0x0F>,
                   m::Pair<OffsetCal4, 0x10>, m::Pair<GainCal1, 0x11>,
                   m::Pair<GainCal2, 0x12>, m::Pair<GainCal3, 0x13>,
                   m::Pair<GainCal4, 0x14>, m::Pair<Manufacturer, 0xFE>,
                   m::Pair<Device, 0xFF>> {};
};

template <m::ifc::CUs TimeUnit, m::ifc::CTime<TimeUnit> Time,
          m::ifc::CIO_Async Io>
class Fdc1004Sync : public IcSync<Fdc1004Sync<TimeUnit, Time, Io>, Fdc1004> {
 public:
  Fdc1004Sync(Time& time, Io& io, TimeUnit add_timeout)
      : time_(time), io_(io), add_timeout_(add_timeout) {}

 private:
  Time& time_;
  Io& io_;
  TimeUnit add_timeout_;
  m::Timeout<TimeUnit> timeout_{time_};

  constexpr static uint8_t Addr = 0x50;

  std::array<volatile uint8_t, 3> read_buf_;
  std::array<uint8_t, 4> write_buf_;

  template <typename Reg>
  bool writeImpl(Reg reg) {
    write_buf_[0] = Addr;
    write_buf_[1] = Fdc1004::Map::template value<Reg>();
    write_buf_[2] = static_cast<uint8_t>(reg.value.getRaw() >> 8);
    write_buf_[3] = static_cast<uint8_t>(reg.value.getRaw());

    return writeSpan(write_buf_);
  }

  template <typename Reg>
  std::optional<Reg> readImpl() {
    write_buf_[0] = Addr;
    write_buf_[1] = Fdc1004::Map::template value<Reg>();

    std::span<const uint8_t> write_span(write_buf_);
    write_span = write_span.first(2);

    if (!writeSpan(write_span)) return std::nullopt;

    read_buf_[0] = Addr;
    read_buf_[1] = 0;
    read_buf_[2] = 0;

    if (!readSpan(read_buf_)) return std::nullopt;

    return getReg<Reg>();
  }

  template <typename Reg>
  Reg getReg() {
    uint16_t raw = (static_cast<uint16_t>(read_buf_[1]) << 8) | read_buf_[2];
    Reg reg{raw};
    return reg;
  }

  bool writeSpan(std::span<const uint8_t> span) {
    if (!io_.writeAsync(span)) return false;

    if (!timeout_.execWithTimeout(
            [&]() { return io_.writeDone(); },
            span.size() * TimeUnit{1'000} / io_.getBaudrate().value() +
                add_timeout_)) {
      return false;
    }

    return true;
  }

  bool readSpan(std::span<volatile uint8_t> span) {
    if (!io_.readAsync(read_buf_)) return false;

    if (!timeout_.execWithTimeout(
            [&]() { return io_.readDone(); },
            span.size() * TimeUnit{1'000} / io_.getBaudrate().value() +
                add_timeout_)) {
      return false;
    }

    return true;
  }

  friend class IcSync<Fdc1004Sync<TimeUnit, Time, Io>, Fdc1004>;
};

namespace detail {
struct Idle : public m::State {};
struct Check : public m::State {};
struct WaitFdcConf : public m::State {};
struct WaitMeas1 : public m::State {};
struct WaitMeas2 : public m::State {};

struct Startup : public m::Event {};
struct Stop : public m::Event {};
struct ReadFdcConf : public m::Event {};
struct ReadMeas1 : public m::Event {};
struct ReadMeas2 : public m::Event {};
struct ReadDone : public m::Event {};

struct Wait : public m::State {};
struct WaitReg : public m::State {};

struct WriteAddr : public m::Event {};
struct ReadReg : public m::Event {};

template <m::ifc::CIO_Async Io>
class FsmReadReg : public m::Fsm_v4<FsmReadReg<Io>, Idle,
                                    m::Transition<Idle, WriteAddr, Wait>,

                                    m::Transition<Wait, ReadReg, WaitReg>,

                                    m::Transition<WaitReg, ReadDone, Idle>

                                    > {
 public:
  FsmReadReg(Io& io) : io_(io) {}

  void handle() { this->checkEvents(); }

  bool start(uint8_t addr) {
    if (!this->template isInState<Idle>()) {
      return false;
    }

    start_ = true;
    addr_ = addr;
    reg_ = std::nullopt;
    return true;
  }

  std::optional<volatile uint16_t> getReg() { return reg_; }

 private:
  Io& io_;

  uint8_t addr_ = 0;
  std::optional<volatile uint16_t> reg_;
  bool start_ = false;

  std::array<volatile uint8_t, 3> read_buf_;
  std::array<uint8_t, 2> write_buf_;

  constexpr static uint8_t Addr = 0x50;

  bool checkEvent(Idle, WriteAddr) { return start_; }
  void handleEvent(Idle, WriteAddr) {
    start_ = false;
    writeAddr(addr_);
  }

  bool checkEvent(Wait, ReadReg) { return io_.writeDone(); }
  void handleEvent(Wait, ReadReg) { readReg(); }

  bool checkEvent(WaitReg, ReadDone) { return io_.readDone(); }
  void handleEvent(WaitReg, ReadDone) {
    reg_ = (static_cast<uint16_t>(read_buf_[1]) << 8) | read_buf_[2];
  }

  // void onEvent(WriteAddr) {
  //   m::DebugLogger<>::getInstance().add("WriteAddr event");
  // }
  // void onEvent(ReadReg) {
  //   m::DebugLogger<>::getInstance().add("ReadReg event");
  // }
  // void onEvent(ReadDone) {
  //   m::DebugLogger<>::getInstance().add("ReadDone event");
  // }

  // void onStateTransition(Idle) {
  //   m::DebugLogger<>::getInstance().add("State: Idle");
  // }
  // void onStateTransition(Wait) {
  //   m::DebugLogger<>::getInstance().add("State: Wait");
  // }
  // void onStateTransition(WaitReg) {
  //   m::DebugLogger<>::getInstance().add("State: WaitReg");
  // }

  friend m::Fsm_v4<FsmReadReg<Io>, Idle, m::Transition<Idle, WriteAddr, Wait>,
                   m::Transition<Wait, ReadReg, WaitReg>,
                   m::Transition<WaitReg, ReadDone, Idle>>;

  void writeAddr(uint8_t reg_addr) {
    write_buf_[0] = Addr;
    write_buf_[1] = reg_addr;

    io_.writeAsync(write_buf_);
  }

  void readReg() {
    read_buf_[0] = Addr;
    read_buf_[1] = 0;
    read_buf_[2] = 0;

    io_.readAsync(read_buf_);
  }
};
}  // namespace detail

template <m::ifc::CIO_Async Io>
class Fdc1004Reader
    : public m::Fsm_v4<
          Fdc1004Reader<Io>, detail::Idle,
          m::Transition<detail::Idle, detail::Startup, detail::Check>,

          m::Transition<detail::Check, detail::Stop, detail::Idle>,
          m::Transition<detail::Check, detail::ReadFdcConf,
                        detail::WaitFdcConf>,

          m::Transition<detail::WaitFdcConf, detail::ReadFdcConf,
                        detail::WaitFdcConf>,
          m::Transition<detail::WaitFdcConf, detail::ReadMeas1,
                        detail::WaitMeas1>,

          m::Transition<detail::WaitMeas1, detail::ReadMeas2,
                        detail::WaitMeas2>,

          m::Transition<detail::WaitMeas2, detail::ReadDone, detail::Check>

          > {
 public:
  Fdc1004Reader(Io& io) : io_(io) {}

  void handle() { this->checkEvents(); }

  bool start(std::span<uint32_t> data) {
    if (!data_.empty()) return false;

    size_ = data.size();
    data_ = data;
    start_ = true;
    return true;
  }

  bool readDone() { return data_.empty(); }

  std::size_t readed() { return size_ - data_.size(); }

 private:
  Io& io_;
  bool start_ = false;
  std::span<uint32_t> data_;
  std::size_t size_ = 0;

  uint32_t meas_ = 0;

  detail::FsmReadReg<Io> fsm_read_reg_{io_};

  // Idle
  bool checkEvent(detail::Idle, detail::Startup) { return start_; }
  void handleEvent(detail::Idle, detail::Startup) { start_ = false; }

  // Check
  bool checkEvent(detail::Check, detail::Stop) { return data_.empty(); }
  void handleEvent(detail::Check, detail::Stop) {}

  bool checkEvent(detail::Check, detail::ReadFdcConf) { return !data_.empty(); }
  void handleEvent(detail::Check, detail::ReadFdcConf) {
    fsm_read_reg_.start(Fdc1004::Map::value<Fdc1004::FdcConf>());
  }

  // WaitFdcConf
  bool checkEvent(detail::WaitFdcConf, detail::ReadFdcConf) {
    fsm_read_reg_.handle();
    if (auto value = fsm_read_reg_.getReg(); value) {
      Fdc1004::FdcConf fdc_conf{value.value()};
      return !fdc_conf.value.get<Fdc1004::FdcConf::Done1>();
    }
    return false;
  }
  void handleEvent(detail::WaitFdcConf, detail::ReadFdcConf) {
    fsm_read_reg_.start(Fdc1004::Map::value<Fdc1004::FdcConf>());
  }

  bool checkEvent(detail::WaitFdcConf, detail::ReadMeas1) {
    fsm_read_reg_.handle();
    if (auto value = fsm_read_reg_.getReg(); value) {
      Fdc1004::FdcConf fdc_conf{value.value()};
      return fdc_conf.value.get<Fdc1004::FdcConf::Done1>();
    }
    return false;
  }
  void handleEvent(detail::WaitFdcConf, detail::ReadMeas1) {
    fsm_read_reg_.start(Fdc1004::Map::value<Fdc1004::Meas1Msb>());
  }

  // WaitMeas1
  bool checkEvent(detail::WaitMeas1, detail::ReadMeas2) {
    fsm_read_reg_.handle();
    return fsm_read_reg_.getReg().has_value();
  }
  void handleEvent(detail::WaitMeas1, detail::ReadMeas2) {
    meas_ = static_cast<uint32_t>(fsm_read_reg_.getReg().value()) << 16;
    fsm_read_reg_.start(Fdc1004::Map::value<Fdc1004::Meas1Lsb>());
  }

  // WaitMeas2
  bool checkEvent(detail::WaitMeas2, detail::ReadDone) {
    fsm_read_reg_.handle();
    return fsm_read_reg_.getReg().has_value();
  }
  void handleEvent(detail::WaitMeas2, detail::ReadDone) {
    meas_ |= static_cast<uint32_t>(fsm_read_reg_.getReg().value());
    meas_ = meas_ >> 8;
    data_[0] = meas_;
    data_ = data_.subspan(1);
  }

  // void onEvent(Startup) {
  //   m::DebugLogger<>::getInstance().add("Startup event");
  // }
  // void onEvent(Stop) { m::DebugLogger<>::getInstance().add("Stop event"); }
  // void onEvent(ReadFdcConf) {
  //   m::DebugLogger<>::getInstance().add("ReadFdcConf event");
  // }
  // void onEvent(ReadMeas1) {
  //   m::DebugLogger<>::getInstance().add("ReadMeas1 event");
  // }
  // void onEvent(ReadMeas2) {
  //   m::DebugLogger<>::getInstance().add("ReadMeas2 event");
  // }

  // void onStateTransition(Idle) {
  //   m::DebugLogger<>::getInstance().add("State: Idle");
  // }
  // void onStateTransition(Check) {
  //   m::DebugLogger<>::getInstance().add("State: Check");
  // }
  // void onStateTransition(WaitFdcConf) {
  //   m::DebugLogger<>::getInstance().add("State: WaitFdcConf");
  // }
  // void onStateTransition(WaitMeas1) {
  //   m::DebugLogger<>::getInstance().add("State: WaitMeas1");
  // }
  // void onStateTransition(WaitMeas2) {
  //   m::DebugLogger<>::getInstance().add("State: WaitMeas2");
  // }

  friend m::Fsm_v4<
      Fdc1004Reader<Io>, detail::Idle,
      m::Transition<detail::Idle, detail::Startup, detail::Check>,

      m::Transition<detail::Check, detail::Stop, detail::Idle>,
      m::Transition<detail::Check, detail::ReadFdcConf, detail::WaitFdcConf>,

      m::Transition<detail::WaitFdcConf, detail::ReadFdcConf,
                    detail::WaitFdcConf>,
      m::Transition<detail::WaitFdcConf, detail::ReadMeas1, detail::WaitMeas1>,

      m::Transition<detail::WaitMeas1, detail::ReadMeas2, detail::WaitMeas2>,

      m::Transition<detail::WaitMeas2, detail::ReadDone, detail::Check>

      >;
};
}  // namespace m::ic

#endif  // FDC1004_HPP