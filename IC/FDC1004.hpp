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

#include <Fsm_v4.hpp>
#include <IIO_Async.hpp>
#include <ITime.hpp>
#include <Ms.hpp>
#include <Reg.hpp>
#include <RegMap.hpp>
#include <Timeout.hpp>
#include <cstdint>
#include <optional>

#include "Fsm.hpp"

namespace m::ic::fdc1004 {

struct MeasMap {
  struct MeasMsbField : public m::BitField<MeasMsbField, 16> {};
  struct MeasLsbField : public m::BitField<MeasLsbField, 8> {};
};

struct Meas1 : public m::Reg<uint32_t, MeasMap, MeasMap::MeasMsbField,
                             MeasMap::MeasLsbField, m::UnusedField<8>> {};
struct Meas2 : public m::Reg<uint32_t, MeasMap, MeasMap::MeasMsbField,
                             MeasMap::MeasLsbField, m::UnusedField<8>> {};
struct Meas3 : public m::Reg<uint32_t, MeasMap, MeasMap::MeasMsbField,
                             MeasMap::MeasLsbField, m::UnusedField<8>> {};
struct Meas4 : public m::Reg<uint32_t, MeasMap, MeasMap::MeasMsbField,
                             MeasMap::MeasLsbField, m::UnusedField<8>> {};

struct ConfMap {
  struct CapdacField : public m::BitField<CapdacField, 5> {};
  struct ChbField : public m::BitField<ChbField, 3> {};
  struct ChaField : public m::BitField<ChaField, 3> {};
};

struct ConfMeas1
    : public m::Reg<uint16_t, ConfMap, m::UnusedField<5>, ConfMap::CapdacField,
                    ConfMap::ChbField, ConfMap::ChaField> {};
struct ConfMeas2
    : public m::Reg<uint16_t, ConfMap, m::UnusedField<5>, ConfMap::CapdacField,
                    ConfMap::ChbField, ConfMap::ChaField> {};
struct ConfMeas3
    : public m::Reg<uint16_t, ConfMap, m::UnusedField<5>, ConfMap::CapdacField,
                    ConfMap::ChbField, ConfMap::ChaField> {};
struct ConfMeas4
    : public m::Reg<uint16_t, ConfMap, m::UnusedField<5>, ConfMap::CapdacField,
                    ConfMap::ChbField, ConfMap::ChaField> {};

struct FdcMap {
  struct Done4Field : public m::BitField<Done4Field, 1> {};
  struct Done3Field : public m::BitField<Done3Field, 1> {};
  struct Done2Field : public m::BitField<Done2Field, 1> {};
  struct Done1Field : public m::BitField<Done1Field, 1> {};
  struct Meas4Field : public m::BitField<Meas4Field, 1> {};
  struct Meas3Field : public m::BitField<Meas3Field, 1> {};
  struct Meas2Field : public m::BitField<Meas2Field, 1> {};
  struct Meas1Field : public m::BitField<Meas1Field, 1> {};
  struct RepeatField : public m::BitField<RepeatField, 1> {};
  struct RateField : public m::BitField<RateField, 2> {};
  struct ResetField : public m::BitField<ResetField, 1> {};
};

struct FdcConf
    : public m::Reg<uint16_t, FdcMap, FdcMap::Done4Field, FdcMap::Done3Field,
                    FdcMap::Done2Field, FdcMap::Done1Field, FdcMap::Meas4Field,
                    FdcMap::Meas3Field, FdcMap::Meas2Field, FdcMap::Meas1Field,
                    FdcMap::RepeatField, m::UnusedField<1>, FdcMap::RateField,
                    m::UnusedField<3>, FdcMap::ResetField> {};

struct Fdc1004Map
    : public m::RegMap<uint8_t, m::RegInfo<0x00, Meas1>,
                       m::RegInfo<0x02, Meas2>, m::RegInfo<0x04, Meas3>,
                       m::RegInfo<0x06, Meas4>, m::RegInfo<0x08, ConfMeas1>,
                       m::RegInfo<0x09, ConfMeas2>, m::RegInfo<0x0A, ConfMeas3>,
                       m::RegInfo<0x0B, ConfMeas4>, m::RegInfo<0x0C, FdcConf>> {

};

namespace {
struct Idle : m::State {};
struct Check : m::State {};
struct WaitFdcFlag : m::State {};
struct WaitMeas : m::State {};

struct Start : m::Event {};
struct Stop : m::Event {};
struct NotReady : m::Event {};

}  // namespace

template <m::ifc::CMs TimeUnit, m::ifc::CTime<TimeUnit> Time,
          m::ifc::CIO_Async Io>
class Fdc1004 : public m::Fsm_v4<Fdc1004<TimeUnit, Time, Io>, Idle,
                                 m::Transition<Idle, Start, Check>,

                                 m::Transition<Check, Stop, Idle>,
                                 m::Transition<Check, Start, WaitFdcFlag>,

                                 m::Transition<WaitFdcFlag, NotReady, Check>,
                                 m::Transition<WaitFdcFlag, Start, WaitMeas>,

                                 m::Transition<WaitMeas, Start, Check>> {
 public:
  Fdc1004(Time& time, Io& io) : time_(time), io_(io) {}

  enum class Cha : uint8_t { Cin1 = 0, Cin2, Cin3, Cin4 };
  enum class Chb : uint8_t {
    Cin1 = 0,
    Cin2,
    Cin3,
    Cin4,
    Capdac,
    Disabled = 0b111
  };
  enum class Rate : uint8_t { R_100Sps = 1, R_200Sps, R_400Sps };

  bool setMeas1(Cha cha, Chb chb, uint8_t capdac) {
    meas_conf_.set<ConfMeas1::Map::ChaField>(static_cast<uint8_t>(cha));
    meas_conf_.set<ConfMeas1::Map::ChbField>(static_cast<uint8_t>(chb));
    meas_conf_.set<ConfMeas1::Map::CapdacField>(capdac);

    return writeReg(map_.getAddress<ConfMeas1>(), meas_conf_.getRaw());
  }

  bool setMode(Rate rate, bool repeat) {
    fdc_conf_.set<FdcConf::Map::RateField>(static_cast<uint8_t>(rate));
    fdc_conf_.set<FdcConf::Map::RepeatField>(repeat);
    fdc_conf_.set<FdcConf::Map::Meas1Field>(1);

    return writeReg(map_.getAddress<FdcConf>(), fdc_conf_.getRaw());
  }

  bool enableMeas(bool value) {
    if (!value) {
      fdc_conf_.set<FdcConf::Map::RepeatField>(0);
    }
    fdc_conf_.set<FdcConf::Map::Meas1Field>(value);
    return writeReg(map_.getAddress<FdcConf>(), fdc_conf_.getRaw());
  }

  bool writeReg(uint8_t addr, uint16_t reg) {
    std::array<uint8_t, 4> buf;
    buf[0] = Addr;
    buf[1] = addr;
    buf[2] = static_cast<uint8_t>(reg >> 8);
    buf[3] = static_cast<uint8_t>(reg);

    if (!io_.writeAsync(buf)) return false;

    if (!timeout_.execWithTimeout(
            [&]() { return io_.writeDone(); },
            buf.size() * TimeUnit{1'000} / io_.getBaudrate().value() +
                TimeUnit{5})) {
      return false;
    }

    return true;
  }

  std::optional<uint16_t> readReg(uint8_t addr) {
    std::array<uint8_t, 5> buf;
    buf[0] = Addr;
    buf[1] = addr;
    buf[2] = Addr;
    buf[3] = 0;
    buf[4] = 0;

    if (!io_.readAsync(buf)) return std::nullopt;

    if (!timeout_.execWithTimeout(
            [&]() { return io_.readDone(); },
            buf.size() * TimeUnit{1'000} / io_.getBaudrate().value() +
                TimeUnit{5})) {
      return std::nullopt;
    }

    uint16_t reg = (static_cast<uint16_t>(buf[3]) << 8) | buf[4];
    return reg;
  }

  bool fillBuf(std::span<uint32_t> buf) {
    buf_ = buf;
    if (!enableMeas(1)) return false;

    start_flag_ = true;

    return true;
  }

  void handle() { this->checkEvents(); }

 private:
  Time& time_;
  Io& io_;

  constexpr static uint8_t Addr = 0x50;

  Fdc1004Map map_;
  ConfMeas1 meas_conf_{0x1C'00};
  FdcConf fdc_conf_;

  m::Timeout<TimeUnit> timeout_{time_};

  std::span<uint32_t> buf_;

  std::array<uint8_t, 7> reg_buf_;

  bool readRegAsync(uint8_t addr) {
    reg_buf_[0] = Addr;
    reg_buf_[1] = addr;
    reg_buf_[2] = Addr;
    reg_buf_[3] = 0;
    reg_buf_[4] = 0;

    auto span = std::span<uint8_t>(reg_buf_).first(5);

    return io_.readAsync(span);
  }

  bool readDataAsync(uint8_t addr) {
    reg_buf_[0] = Addr;
    reg_buf_[1] = addr;
    reg_buf_[2] = Addr;
    reg_buf_[3] = 0;
    reg_buf_[4] = 0;
    reg_buf_[5] = 0;
    reg_buf_[6] = 0;

    return io_.readAsync(reg_buf_);
  }

  bool readDone() { return io_.readDone(); }

  uint16_t getRegValue() {
    return (static_cast<uint16_t>(reg_buf_[3]) << 8) | reg_buf_[4];
  }

  uint32_t getDataValue() {
    return (static_cast<uint32_t>(reg_buf_[3]) << 24) |
           (static_cast<uint32_t>(reg_buf_[4]) << 16) |
           (static_cast<uint32_t>(reg_buf_[5]) << 8) | reg_buf_[6];
  }

  // #########################
  //          Idle
  // #########################

  bool start_flag_ = false;

  bool checkEvent(Idle, Start) { return start_flag_; }
  void handleEvent(Idle, Start) { start_flag_ = false; }

  // #########################
  //          Check
  // #########################

  bool checkEvent(Check, Stop) { return buf_.empty(); }
  void handleEvent(Check, Stop) {}

  bool checkEvent(Check, Start) { return !buf_.empty(); }
  void handleEvent(Check, Start) {
    if (!readRegAsync(map_.getAddress<FdcConf>())) {
      // TODO: log
    }
  }

  // #########################
  //       WaitFdcFlag
  // #########################
  bool checkEvent(WaitFdcFlag, NotReady) {
    if (readDone()) {
      auto reg = getRegValue();
      FdcConf fdc{reg};
      return !fdc.get<FdcConf::Map::Done1Field>();
    }
    return false;
  }
  void handleEvent(WaitFdcFlag, NotReady) {}

  bool checkEvent(WaitFdcFlag, Start) {
    if (readDone()) {
      auto reg = getRegValue();
      FdcConf fdc{reg};
      return fdc.get<FdcConf::Map::Done1Field>();
    }
    return false;
  }
  void handleEvent(WaitFdcFlag, Start) {
    if (!readDataAsync(map_.getAddress<Meas1>())) {
      // TODO: log
    }
  }

  // #########################
  //        WaitMeas
  // #########################
  bool checkEvent(WaitMeas, Start) { return readDone(); }
  void handleEvent(WaitMeas, Start) {
    auto meas1 = getDataValue();
    if (!buf_.empty()) {
      buf_[0] = meas1;
      buf_ = buf_.subspan(1);
    }
  }

  friend class m::Fsm_v4<Fdc1004<TimeUnit, Time, Io>, Idle,
                         m::Transition<Idle, Start, Check>,

                         m::Transition<Check, Stop, Idle>,
                         m::Transition<Check, Start, WaitFdcFlag>,

                         m::Transition<WaitFdcFlag, NotReady, Check>,
                         m::Transition<WaitFdcFlag, Start, WaitMeas>,

                         m::Transition<WaitMeas, Start, Check>>;
};
}  // namespace m::ic::fdc1004
#endif  // FDC1004_HPP