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
#include <IcSync.hpp>
#include <Reg.hpp>
#include <StaticMap.hpp>
#include <Timeout.hpp>
#include <Us.hpp>
#include <cstdint>

namespace m::ic {

struct Fdc1004 {
  struct Meas1 {
    struct MeasMsb : public m::BitField<MeasMsb, 16> {};
    struct MeasLsb : public m::BitField<MeasLsb, 8> {};

    m::Reg<uint32_t, MeasMsb, MeasLsb, m::UnusedField<8>> value;
  };
  struct Meas2 {
    struct MeasMsb : public m::BitField<MeasMsb, 16> {};
    struct MeasLsb : public m::BitField<MeasLsb, 8> {};

    m::Reg<uint32_t, MeasMsb, MeasLsb, m::UnusedField<8>> value;
  };
  struct Meas3 {
    struct MeasMsb : public m::BitField<MeasMsb, 16> {};
    struct MeasLsb : public m::BitField<MeasLsb, 8> {};

    m::Reg<uint32_t, MeasMsb, MeasLsb, m::UnusedField<8>> value;
  };
  struct Meas4 {
    struct MeasMsb : public m::BitField<MeasMsb, 16> {};
    struct MeasLsb : public m::BitField<MeasLsb, 8> {};

    m::Reg<uint32_t, MeasMsb, MeasLsb, m::UnusedField<8>> value;
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

  using Regs = std::tuple<Meas1, Meas2, Meas3, Meas4, ConfMeas1, ConfMeas2,
                          ConfMeas3, ConfMeas4, FdcConf, OffsetCal1, OffsetCal2,
                          OffsetCal3, OffsetCal4, GainCal1, GainCal2, GainCal3,
                          GainCal4, Manufacturer, Device>;

  struct Map : public m::StaticMap<
                   uint8_t, m::Pair<Meas1, 0x00>, m::Pair<Meas2, 0x02>,
                   m::Pair<Meas3, 0x04>, m::Pair<Meas4, 0x06>,
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
class Fdc1004Ic : public IcSync<Fdc1004Ic<TimeUnit, Time, Io>, TimeUnit, Time,
                                Io, Fdc1004> {
 public:
  Fdc1004Ic(Time& time, Io& io, TimeUnit add_timeout)
      : IcSync<Fdc1004Ic<TimeUnit, Time, Io>, TimeUnit, Time, Io, Fdc1004>(
            time, io, add_timeout) {}

 private:
  constexpr static uint8_t Addr = 0x50;

  std::array<uint8_t, 5> read_buf_;
  std::array<uint8_t, 7> read_buf_large_;
  std::array<uint8_t, 4> write_buf_;

  template <typename Reg>
  std::span<uint8_t> getWriteBuf(Reg reg) {
    write_buf_[0] = Addr;
    write_buf_[1] = Fdc1004::Map::template value<Reg>();
    write_buf_[2] = static_cast<uint8_t>(reg.value.getRaw() >> 8);
    write_buf_[3] = static_cast<uint8_t>(reg.value.getRaw());
    return write_buf_;
  }

  template <typename Reg>
  std::span<volatile uint8_t> getReadBuf() {
    read_buf_[0] = Addr;
    read_buf_[1] = Fdc1004::Map::template value<Reg>();
    read_buf_[2] = Addr;
    read_buf_[3] = 0;
    read_buf_[4] = 0;

    return read_buf_;
  }

  template <typename Reg>
    requires std::same_as<Reg, Fdc1004::Meas1> ||
             std::same_as<Reg, Fdc1004::Meas2> ||
             std::same_as<Reg, Fdc1004::Meas3> ||
             std::same_as<Reg, Fdc1004::Meas4>
  std::span<volatile uint8_t> getReadBuf() {
    read_buf_large_[0] = Addr;
    read_buf_large_[1] = Fdc1004::Map::template value<Reg>();
    read_buf_large_[2] = Addr;
    read_buf_large_[3] = 0;
    read_buf_large_[4] = 0;
    read_buf_large_[5] = 0;
    read_buf_large_[6] = 0;

    return read_buf_large_;
  }

  template <typename Reg>
  Reg getReg() {
    uint16_t raw = (static_cast<uint16_t>(read_buf_[3]) << 8) | read_buf_[4];
    Reg reg{raw};
    return reg;
  }

  template <typename Reg>
    requires std::same_as<Reg, Fdc1004::Meas1> ||
             std::same_as<Reg, Fdc1004::Meas2> ||
             std::same_as<Reg, Fdc1004::Meas3> ||
             std::same_as<Reg, Fdc1004::Meas4>
  Reg getReg() {
    uint32_t raw = (static_cast<uint32_t>(read_buf_large_[3]) << 24) |
                   (static_cast<uint32_t>(read_buf_large_[4]) << 16) |
                   (static_cast<uint32_t>(read_buf_large_[5]) << 8) |
                   (static_cast<uint32_t>(read_buf_large_[6]));
    Reg reg{raw};
    return reg;
  }

  friend class IcSync<Fdc1004Ic<TimeUnit, Time, Io>, TimeUnit, Time, Io,
                      Fdc1004>;
};

// namespace {
// struct Idle : m::State {};
// struct Check : m::State {};
// struct WaitFdcFlag : m::State {};
// struct WaitMeas : m::State {};

// struct Start : m::Event {};
// struct Stop : m::Event {};
// struct NotReady : m::Event {};

// }  // namespace

// Fdc1004Map map_;
// ConfMeas1 meas_conf_{0x1C'00};
// FdcConf fdc_conf_;

// std::span<uint32_t> buf_;

// std::array<uint8_t, 7> reg_buf_;

// bool readRegAsync(uint8_t addr) {
//   reg_buf_[0] = Addr;
//   reg_buf_[1] = addr;
//   reg_buf_[2] = Addr;
//   reg_buf_[3] = 0;
//   reg_buf_[4] = 0;

//   auto span = std::span<uint8_t>(reg_buf_).first(5);

//   return io_.readAsync(span);
// }

// bool readDataAsync(uint8_t addr) {
//   reg_buf_[0] = Addr;
//   reg_buf_[1] = addr;
//   reg_buf_[2] = Addr;
//   reg_buf_[3] = 0;
//   reg_buf_[4] = 0;
//   reg_buf_[5] = 0;
//   reg_buf_[6] = 0;

//   return io_.readAsync(reg_buf_);
// }

// bool readDone() { return io_.readDone(); }

// uint16_t getRegValue() {
//   return (static_cast<uint16_t>(reg_buf_[3]) << 8) | reg_buf_[4];
// }

// uint32_t getDataValue() {
//   return (static_cast<uint32_t>(reg_buf_[3]) << 24) |
//          (static_cast<uint32_t>(reg_buf_[4]) << 16) |
//          (static_cast<uint32_t>(reg_buf_[5]) << 8) | reg_buf_[6];
// }

// bool setMeas1(Cha cha, Chb chb, uint8_t capdac) {
//   meas_conf_.set<ConfMeas1::Map::Cha>(static_cast<uint8_t>(cha));
//   meas_conf_.set<ConfMeas1::Map::Chb>(static_cast<uint8_t>(chb));
//   meas_conf_.set<ConfMeas1::Map::Capdac>(capdac);

//   return writeReg(map_.getAddress<ConfMeas1>(), meas_conf_.getRaw());
// }

// bool setMode(Rate rate, bool repeat) {
//   fdc_conf_.set<FdcConf::Map::Rate>(static_cast<uint8_t>(rate));
//   fdc_conf_.set<FdcConf::Map::Repeat>(repeat);
//   fdc_conf_.set<FdcConf::Map::Meas1>(1);

//   return writeReg(map_.getAddress<FdcConf>(), fdc_conf_.getRaw());
// }

// bool enableMeas(bool value) {
//   if (!value) {
//     fdc_conf_.set<FdcConf::Map::Repeat>(0);
//   }
//   fdc_conf_.set<FdcConf::Map::Meas1>(value);
//   return writeReg(map_.getAddress<FdcConf>(), fdc_conf_.getRaw());
// }

// class Fdc1004Reader
//     : public m::Fsm_v4<Fdc1004Reader, Idle, m::Transition<Idle, Start,
//     Check>,

//                        m::Transition<Check, Stop, Idle>,
//                        m::Transition<Check, Start, WaitFdcFlag>,

//                        m::Transition<WaitFdcFlag, NotReady, Check>,
//                        m::Transition<WaitFdcFlag, Start, WaitMeas>,

//                        m::Transition<WaitMeas, Start, Check>> {
//  public:
//   void handle() { this->checkEvents(); }

//  private:
//   // #########################
//   //          Idle
//   // #########################

//   bool start_flag_ = false;

//   bool checkEvent(Idle, Start) { return start_flag_; }
//   void handleEvent(Idle, Start) { start_flag_ = false; }

//   // #########################
//   //          Check
//   // #########################

//   bool checkEvent(Check, Stop) { return buf_.empty(); }
//   void handleEvent(Check, Stop) {}

//   bool checkEvent(Check, Start) { return !buf_.empty(); }
//   void handleEvent(Check, Start) {
//     if (!readRegAsync(map_.getAddress<FdcConf>())) {
//       // TODO: log
//     }
//   }

//   // #########################
//   //       WaitFdcFlag
//   // #########################
//   bool checkEvent(WaitFdcFlag, NotReady) {
//     if (readDone()) {
//       autovalue = getRegValue();
//       FdcConf fdc{reg};
//       return !fdc.get<FdcConf::Map::Done1>();
//     }
//     return false;
//   }
//   void handleEvent(WaitFdcFlag, NotReady) {}

//   bool checkEvent(WaitFdcFlag, Start) {
//     if (readDone()) {
//       autovalue = getRegValue();
//       FdcConf fdc{reg};
//       return fdc.get<FdcConf::Map::Done1>();
//     }
//     return false;
//   }
//   void handleEvent(WaitFdcFlag, Start) {
//     if (!readDataAsync(map_.getAddress<Meas1>())) {
//       // TODO: log
//     }
//   }

//   // #########################
//   //        WaitMeas
//   // #########################
//   bool checkEvent(WaitMeas, Start) { return readDone(); }
//   void handleEvent(WaitMeas, Start) {
//     auto meas1 = getDataValue();
//     if (!buf_.empty()) {
//       buf_[0] = meas1;
//       buf_ = buf_.subspan(1);
//     }
//   }

//   friend class m::Fsm_v4<Fdc1004Reader, Idle, m::Transition<Idle, Start,
//   Check>,

//                          m::Transition<Check, Stop, Idle>,
//                          m::Transition<Check, Start, WaitFdcFlag>,

//                          m::Transition<WaitFdcFlag, NotReady, Check>,
//                          m::Transition<WaitFdcFlag, Start, WaitMeas>,

//                          m::Transition<WaitMeas, Start, Check>>;
// };
}  // namespace m::ic
#endif  // FDC1004_HPP