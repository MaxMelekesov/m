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

#include <BitReg.hpp>
#include <CIO_Async.hpp>
#include <Ms.hpp>
#include <RegMap.hpp>

namespace m::ic {

template <m::c::CIO_Async Io>
class Fdc1004 {
 public:
  Fdc1004(Io& io) : io_(io) {}

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

 private:
  Io& io_;

  struct MeasMsbField : public m::BitField<16, MeasMsbField> {};
  struct MeasLsbField : public m::BitField<8, MeasLsbField> {};

  struct Meas1
      : public m::BitReg<uint32_t, MeasMsbField, MeasLsbField, DummyField<8>> {
  };
  struct Meas2
      : public m::BitReg<uint32_t, MeasMsbField, MeasLsbField, DummyField<8>> {
  };
  struct Meas3
      : public m::BitReg<uint32_t, MeasMsbField, MeasLsbField, DummyField<8>> {
  };
  struct Meas4
      : public m::BitReg<uint32_t, MeasMsbField, MeasLsbField, DummyField<8>> {
  };

  struct ChaField : public m::BitField<3, ChaField> {};
  struct ChbField : public m::BitField<3, ChbField> {};
  struct CapdacField : public m::BitField<3, CapdacField> {};

  struct ConfMeas1 : public m::BitReg<uint16_t, DummyField<5>, CapdacField,
                                      ChbField, ChaField> {};
  struct ConfMeas2 : public m::BitReg<uint16_t, DummyField<5>, CapdacField,
                                      ChbField, ChaField> {};
  struct ConfMeas3 : public m::BitReg<uint16_t, DummyField<5>, CapdacField,
                                      ChbField, ChaField> {};
  struct ConfMeas4 : public m::BitReg<uint16_t, DummyField<5>, CapdacField,
                                      ChbField, ChaField> {};

  struct Done4Field : public m::BitField<1, Done4Field> {};
  struct Done3Field : public m::BitField<1, Done3Field> {};
  struct Done2Field : public m::BitField<1, Done2Field> {};
  struct Done1Field : public m::BitField<1, Done1Field> {};
  struct Meas4Field : public m::BitField<1, Meas4Field> {};
  struct Meas3Field : public m::BitField<1, Meas3Field> {};
  struct Meas2Field : public m::BitField<1, Meas2Field> {};
  struct Meas1Field : public m::BitField<1, Meas1Field> {};
  struct RepeatField : public m::BitField<1, RepeatField> {};
  struct RateField : public m::BitField<2, RateField> {};

  struct FdcConf
      : public m::BitReg<uint16_t, Done4Field, Done3Field, Done2Field,
                         Done1Field, Meas4Field, Meas3Field, Meas2Field,
                         Meas1Field, RepeatField, m::DummyField<1>, RateField> {
  };

  class Fdc1004_Regs
      : public m::RegMap<
            Fdc1004_Regs, uint8_t, m::RegInfo<0x00, Meas1>,
            m::RegInfo<0x02, Meas2>, m::RegInfo<0x04, Meas3>,
            m::RegInfo<0x06, Meas4>, m::RegInfo<0x08, ConfMeas1>,
            m::RegInfo<0x09, ConfMeas2>, m::RegInfo<0x0A, ConfMeas3>,
            m::RegInfo<0x0B, ConfMeas4>, m::RegInfo<0x0C, FdcConf>> {
   public:
    Fdc1004_Regs(Io& io) : io_(io) {}

   private:
    Io& io_;

    template <typename RegType>
    RegType getImpl(uint8_t address) {
      if constexpr (sizeof(RegType) == 4) {
        RegType{};
      } else {
        return RegType{};
      }
    }

    FdcConf getImpl(uint8_t address) { return FdcConf{}; }

    template <typename RegisterType>
    bool setImpl(uint8_t address, const RegisterType& reg) {
      return false;
    }

    friend m::RegMap<Fdc1004_Regs, uint8_t, m::RegInfo<0x00, Meas1>,
                     m::RegInfo<0x02, Meas2>, m::RegInfo<0x04, Meas3>,
                     m::RegInfo<0x06, Meas4>, m::RegInfo<0x08, ConfMeas1>,
                     m::RegInfo<0x09, ConfMeas2>, m::RegInfo<0x0A, ConfMeas3>,
                     m::RegInfo<0x0B, ConfMeas4>, m::RegInfo<0x0C, FdcConf>>;
  };

  Fdc1004_Regs ic_map_{io_};
};
}  // namespace m::ic

//  private:
//   m::ifc::IIO_Sync<Ms<int>>& i2c_;
//   uint8_t addr_ = 0x50;

//  public:
//   FDC1004(m::ifc::IIO_Sync<Ms<int>>& i2c) : i2c_(i2c) {}

//   enum class RegAddr : uint8_t {
//     Meas_1_Msb = 0,
//     Meas_1_Lsb,
//     Meas_2_Msb,
//     Meas_2_Lsb,
//     Meas_3_Msb,
//     Meas_3_Lsb,
//     Meas_4_Msb,
//     Meas_4_Lsb,
//     Meas_Conf_1,
//     Meas_Conf_2,
//     Meas_Conf_3,
//     Meas_Conf_4,
//     Fdc_Conf,
//   };

//   enum class MeasIndex : uint8_t {
//     N_1 = 1,
//     N_2,
//     N_3,
//     N_4,
//   };

//   enum class ChA : uint8_t {
//     CIN1 = 0,
//     CIN2,
//     CIN3,
//     CIN4,
//   };

//   enum class ChB : uint8_t {
//     CIN1 = 0,
//     CIN2,
//     CIN3,
//     CIN4,
//     CAPDAC = 0b100,
//     DISABLED = 0b111
//   };

//   // BitReg fields for MeasConf register
//   struct ChaField : public m::BitField<3, ChaField, 0> {};  // Bits 0-2: CHA
//   struct ChbField : public m::BitField<3, ChbField, 0> {};  // Bits 3-5: CHB
//   struct CapdacField : public m::BitField<5, CapdacField, 0> {
//   };  // Bits 6-10: CAPDAC
//   // Bits 11-15 are reserved

//   using MeasConfRegister =
//       m::Register<std::uint16_t, ChaField, ChbField, CapdacField,
//                   m::DummyField<5>>;  // Reserved bits 11-15

//   enum class Rate : uint8_t { R_100 = 1, R_200, R_400 };

//   // BitReg fields for FdcConf register
//   struct RstField : public m::BitField<1, RstField, 0> {};  // Bit 15: Reset
//   struct Rate1Field : public m::BitField<1, Rate1Field, 0> {
//   };  // Bit 10: Rate bit 1
//   struct Rate0Field : public m::BitField<1, Rate0Field, 0> {
//   };  // Bit 11: Rate bit 0
//   struct RepeatField : public m::BitField<1, RepeatField, 0> {
//   };  // Bit 8: Repeat
//   struct Meas1Field : public m::BitField<1, Meas1Field, 0> {};  // Bit 7:
//   Meas 1 struct Meas2Field : public m::BitField<1, Meas2Field, 0> {};  // Bit
//   6: Meas 2 struct Meas3Field : public m::BitField<1, Meas3Field, 0> {};  //
//   Bit 5: Meas 3 struct Meas4Field : public m::BitField<1, Meas4Field, 0> {};
//   // Bit 4: Meas 4 struct Done1Field : public m::BitField<1, Done1Field, 0>
//   {};  // Bit 3: Done 1 struct Done2Field : public m::BitField<1, Done2Field,
//   0> {};  // Bit 2: Done 2 struct Done3Field : public m::BitField<1,
//   Done3Field, 0> {};  // Bit 1: Done 3 struct Done4Field : public
//   m::BitField<1, Done4Field, 0> {};  // Bit 0: Done 4

//   using FdcConfRegister = m::Register<std::uint16_t,
//                                       Done4Field,        // Bit 0
//                                       Done3Field,        // Bit 1
//                                       Done2Field,        // Bit 2
//                                       Done1Field,        // Bit 3
//                                       Meas4Field,        // Bit 4
//                                       Meas3Field,        // Bit 5
//                                       Meas2Field,        // Bit 6
//                                       Meas1Field,        // Bit 7
//                                       RepeatField,       // Bit 8
//                                       m::DummyField<1>,  // Bit 9: Reserved
//                                       Rate1Field,        // Bit 10: Rate[1]
//                                       Rate0Field,        // Bit 11: Rate[0]
//                                       m::DummyField<3>,  // Bits 12-14:
//                                       Reserved RstField>;         // Bit 15

//   static_assert(sizeof(FdcConfRegister) == 2, "FdcConfRegister size
//   mismatch");

//   bool writeReg(RegAddr reg_addr, uint16_t value) {
//     std::array<uint8_t, 4> data{addr_, (uint8_t)reg_addr, (uint8_t)(value >>
//     8),
//                                 (uint8_t)value};

//     auto res =
//         i2c_.write(data, Ms<int>{static_cast<int>(
//                              1'000 * data.size() / i2c_.getBaudrate() + 1)});

//     return res;
//   }

//   std::optional<uint16_t> readReg(RegAddr reg_addr) {
//     std::array<uint8_t, 3> data{addr_, (uint8_t)reg_addr, 0};

//     {
//       auto temp_span = std::span{data}.first(2);
//       if (!i2c_.write(
//               temp_span,
//               Ms<int>{static_cast<int>(
//                   1'000 * temp_span.size() / i2c_.getBaudrate() + 1)})) {
//         return std::nullopt;
//       }
//     }

//     data[1] = 0;
//     if (i2c_.read(data, Ms<int>{static_cast<int>(
//                             1'000 * data.size() / i2c_.getBaudrate() + 1)}))
//                             {
//       uint16_t temp = (((uint16_t)data[1]) << 8) + data[2];
//       return temp;
//     } else {
//       return std::nullopt;
//     }
//   }

//   enum class MeasChannel : uint8_t { N1 = 0, N2, N3, N4 };
//   std::optional<int32_t> readMeas(MeasChannel channel) {
//     std::array<uint8_t, 2> data_tx{addr_, 0};
//     std::array<uint8_t, 3> data_rx{addr_, 0, 0};
//     switch (channel) {
//       case MeasChannel::N1:
//         data_tx[1] = (uint8_t)RegAddr::Meas_1_Msb;
//         break;
//       case MeasChannel::N2:
//         data_tx[1] = (uint8_t)RegAddr::Meas_2_Msb;
//         break;
//       case MeasChannel::N3:
//         data_tx[1] = (uint8_t)RegAddr::Meas_3_Msb;
//         break;
//       case MeasChannel::N4:
//         data_tx[1] = (uint8_t)RegAddr::Meas_4_Msb;
//         break;
//       default:
//         return std::nullopt;
//         break;
//     }

//     if (!i2c_.write(data_tx,
//                     Ms<int>{static_cast<int>(
//                         1'000 * data_tx.size() / i2c_.getBaudrate() + 1)})) {
//       return std::nullopt;
//     }

//     int32_t temp{0};

//     if (i2c_.read(data_rx,
//                   Ms<int>{static_cast<int>(
//                       1'000 * data_rx.size() / i2c_.getBaudrate() + 1)})) {
//       temp = (((uint16_t)data_rx[1]) << 16) + (((uint16_t)data_rx[2]) << 8);
//     } else {
//       return std::nullopt;
//     }

//     ++data_tx[1];
//     if (!i2c_.write(data_tx,
//                     Ms<int>{static_cast<int>(
//                         1'000 * data_tx.size() / i2c_.getBaudrate() + 1)})) {
//       return std::nullopt;
//     }

//     if (i2c_.read(data_rx,
//                   Ms<int>{static_cast<int>(
//                       1'000 * data_rx.size() / i2c_.getBaudrate() + 1)})) {
//       temp += data_rx[1];
//       if (temp & 0x80'00'00) {
//         temp |= 0xFF'00'00'00;
//       }
//       return temp;
//     } else {
//       return std::nullopt;
//     }
//   }

//   bool startMeasurement(MeasChannel channel, ChA cha, ChB chb, uint16_t
//   capdac,
//                         Rate rate) {
//     // Configure measurement register
//     MeasConfRegister meas_conf;
//     meas_conf.set<ChaField>(static_cast<uint8_t>(cha));
//     meas_conf.set<ChbField>(static_cast<uint8_t>(chb));
//     meas_conf.set<CapdacField>(capdac & 0x1F);  // Ensure 5-bit value

//     // Configure FDC register
//     FdcConfRegister fdc_conf;

//     // Set rate (2-bit value)
//     uint8_t rate_val = static_cast<uint8_t>(rate);
//     fdc_conf.set<Rate0Field>(rate_val & 0x01);         // Bit 0 of rate
//     fdc_conf.set<Rate1Field>((rate_val >> 1) & 0x01);  // Bit 1 of rate
//     fdc_conf.set<RepeatField>(1);

//     RegAddr reg_addr = RegAddr::Meas_Conf_1;

//     switch (channel) {
//       case MeasChannel::N1:
//         fdc_conf.set<Meas1Field>(1);
//         reg_addr = RegAddr::Meas_Conf_1;
//         break;
//       case MeasChannel::N2:
//         fdc_conf.set<Meas2Field>(1);
//         reg_addr = RegAddr::Meas_Conf_2;
//         break;
//       case MeasChannel::N3:
//         fdc_conf.set<Meas3Field>(1);
//         reg_addr = RegAddr::Meas_Conf_3;
//         break;
//       case MeasChannel::N4:
//         fdc_conf.set<Meas4Field>(1);
//         reg_addr = RegAddr::Meas_Conf_4;
//         break;
//       default:
//         return false;
//     }

//     if (!writeReg(reg_addr, meas_conf.raw())) return false;
//     if (!writeReg(RegAddr::Fdc_Conf, fdc_conf.raw())) return false;
//     return true;
//   }
// };
// }  // namespace m::ic
#endif  // FDC1004_HPP