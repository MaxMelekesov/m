/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */
#ifndef ADS1256_HPP
#define ADS1256_HPP

#include <IIO_Async.hpp>
#include <IIt.hpp>
#include <IPin.hpp>
#include <ITime.hpp>
#include <Ic.hpp>
#include <Reg.hpp>
#include <StaticMap.hpp>
#include <Timeout.hpp>
#include <Us.hpp>
#include <cstdint>

namespace m::ic {

struct Ads1256 {
  struct Data {
    struct Value : public m::BitField<Value, 24> {};

    m::Reg<uint32_t, Value, m::UnusedField<8>> value;
  };

  struct ReadStatus {
    struct Drdy : public m::BitField<Drdy, 1> {};
    struct Bufen : public m::BitField<Bufen, 1> {
      enum : uint8_t { Off = 0, On };
    };
    struct Acal : public m::BitField<Acal, 1> {
      enum : uint8_t { Disable = 0, Enable };
    };
    struct Order : public m::BitField<Order, 1> {
      enum : uint8_t { Msb = 0, Lsb };
    };
    struct Id : public m::BitField<Id, 4> {};

    m::Reg<uint8_t, Drdy, Bufen, Acal, Order, Id> value{0x01};
  };
  struct WriteStatus : public ReadStatus {};

  struct ReadMux {
    struct Nsel : public m::BitField<Nsel, 4> {
      enum : uint8_t {
        Ain_0 = 0,
        Ain_1 = 1,
        Ain_2 = 2,
        Ain_3 = 3,
        Ain_4 = 4,
        Ain_5 = 5,
        Ain_6 = 6,
        Ain_7 = 7,
        Acom = 8
      };
    };
    struct Psel : public m::BitField<Psel, 4> {
      enum : uint8_t {
        Ain_0 = 0,
        Ain_1 = 1,
        Ain_2 = 2,
        Ain_3 = 3,
        Ain_4 = 4,
        Ain_5 = 5,
        Ain_6 = 6,
        Ain_7 = 7,
        Acom = 8
      };
    };

    m::Reg<uint8_t, Nsel, Psel> value{0x01};
  };
  struct WriteMux : public ReadMux {};

  struct ReadAdcon {
    struct Pga : public m::BitField<Pga, 3> {
      enum : uint8_t {
        G_1 = 0,
        G_2 = 1,
        G_4 = 2,
        G_8 = 3,
        G_16 = 4,
        G_32 = 5,
        G_64 = 6
      };
    };
    struct Sdcs : public m::BitField<Sdcs, 2> {
      enum : uint8_t { Off = 0, I_0_5_uA = 1, I_2_uA = 2, I_10_uA = 3 };
    };
    struct Clk : public m::BitField<Clk, 2> {
      enum : uint8_t {
        Off = 0,
        Full_Freq = 1,
        Half_Freq = 2,
        Quarter_Freq = 3
      };
    };

    m::Reg<uint8_t, Pga, Sdcs, Clk, m::UnusedField<1>> value{0x20};
  };
  struct WriteAdcon : public ReadAdcon {};

  struct ReadDrate {
    struct Dr : public m::BitField<Dr, 8> {
      enum : uint8_t {
        R_30000 = 0b1111'0000,
        R_15000 = 0b1110'0000,
        R_7500 = 0b1101'0000,
        R_3750 = 0b1100'0000,
        R_2000 = 0b1011'0000,
        R_1000 = 0b1010'0001,
        R_500 = 0b1001'0010,
        R_100 = 0b1000'0010,
        R_60 = 0b0111'0010,
        R_50 = 0b0110'0011,
        R_30 = 0b0101'0011,
        R_25 = 0b0100'0011,
        R_15 = 0b0011'0011,
        R_10 = 0b0010'0011,
        R_5 = 0b0001'0011,
        R_2_5 = 0b0000'0011
      };
    };

    m::Reg<uint8_t, Dr> value{0xF0};
  };
  struct WriteDrate : public ReadDrate {};

  struct Selfcal {
    m::Reg<uint8_t, m::UnusedField<8>> value;
  };

  using Regs =
      std::tuple<Data, ReadStatus, ReadMux, ReadAdcon, ReadDrate, WriteStatus,
                 WriteMux, WriteAdcon, WriteDrate, Selfcal>;

  using Map =
      m::StaticMap<uint16_t, m::Pair<Data, 0x01>, m::Pair<ReadStatus, 0x10>,
                   m::Pair<ReadMux, 0x11>, m::Pair<ReadAdcon, 0x12>,
                   m::Pair<ReadDrate, 0x13>, m::Pair<WriteStatus, 0x50>,
                   m::Pair<WriteMux, 0x51>, m::Pair<WriteAdcon, 0x52>,
                   m::Pair<WriteDrate, 0x53>, m::Pair<Selfcal, 0xF0>>;
};

template <m::ifc::CUs Us, m::ifc::CTime<Us> Time, m::ifc::CIO_Async Io>
class Ads1256Ic : public Ic<Ads1256Ic<Us, Time, Io>, Ads1256> {
 public:
  Ads1256Ic(Time& time, Io& io, m::ifc::mcu::IPin& cs, Us add_timeout)
      : time_(time), io_(io), cs_(cs), add_timeout_(add_timeout) {}

 private:
  Time& time_;
  Io& io_;
  m::ifc::mcu::IPin& cs_;
  Us add_timeout_;

  std::array<uint8_t, 3> read_buf_;
  std::array<uint8_t, 3> write_buf_;

  template <typename Reg>
    requires std::is_same_v<Reg, Ads1256::WriteStatus> ||
             std::is_same_v<Reg, Ads1256::WriteMux> ||
             std::is_same_v<Reg, Ads1256::WriteAdcon> ||
             std::is_same_v<Reg, Ads1256::WriteDrate>
  bool writeImpl(Reg reg) {
    write_buf_[0] = Ads1256::Map::template value<Reg>();
    write_buf_[1] = 0;  // register count -1
    write_buf_[2] = static_cast<uint8_t>(reg.value.getRaw());

    auto write_span = std::span<const uint8_t>(write_buf_).first(3);
    return writeSpan(write_span);
  }

  template <typename Reg>
    requires std::is_same_v<Reg, Ads1256::ReadStatus> ||
             std::is_same_v<Reg, Ads1256::ReadMux> ||
             std::is_same_v<Reg, Ads1256::ReadAdcon> ||
             std::is_same_v<Reg, Ads1256::ReadDrate>
  std::optional<Reg> readImpl() {
    write_buf_[0] = Ads1256::Map::template value<Reg>();
    write_buf_[1] = 0;  // register count -1

    auto write_span = std::span<const uint8_t>(write_buf_).first(2);
    if (!writeSpan(write_span)) return std::nullopt;

    time_.delay(Us{7});  // t6 datasheet

    read_buf_[0] = 0;
    auto read_span = std::span<uint8_t>(read_buf_).first(1);
    if (!readSpan(read_span)) return std::nullopt;

    Reg reg{read_buf_[0]};
    return reg;
  }

  std::optional<Ads1256::Data> readImpl() {
    write_buf_[0] = Ads1256::Map::value<Ads1256::Data>();

    auto write_span = std::span<const uint8_t>(write_buf_).first(1);
    if (!writeSpan(write_span)) return std::nullopt;

    time_.delay(Us{7});  // t6 datasheet

    read_buf_.fill(0);
    auto read_span = std::span<uint8_t>(read_buf_).first(3);
    if (!readSpan(read_span)) return std::nullopt;
    uint32_t raw = (static_cast<uint32_t>(read_buf_[0]) << 16) |
                   (static_cast<uint32_t>(read_buf_[1]) << 8) |
                   (static_cast<uint32_t>(read_buf_[2]));

    Ads1256::Data reg{raw};
    return reg;
  }

  template <typename Reg>
  bool writeImpl(Reg reg) {
    write_buf_[0] = Ads1256::Map::template value<Reg>();

    auto write_span = std::span<const uint8_t>(write_buf_).first(1);
    return writeSpan(write_span);
  }

  bool writeSpan(std::span<const uint8_t> span) {
    cs_.write(1);
    if (!io_.writeAsync(span)) return false;

    if (!m::execWithTimeout(
            time_, [&]() { return io_.writeDone(); },
            span.size() * Us{1'000} / io_.getBaudrate().value() +
                add_timeout_)) {
      cs_.write(0);
      return false;
    }
    cs_.write(0);
    return true;
  }

  bool readSpan(std::span<uint8_t> span) {
    cs_.write(1);
    if (!io_.readAsync(read_buf_)) return false;

    if (!m::execWithTimeout(
            time_, [&]() { return io_.readDone(); },
            span.size() * Us{1'000} / io_.getBaudrate().value() +
                add_timeout_)) {
      cs_.write(0);
      return false;
    }
    cs_.write(0);
    return true;
  }

  friend class Ic<Ads1256Ic<Us, Time, Io>, Ads1256>;
};

template <m::ifc::CUs Us, m::ifc::CTime<Us> Time, m::ifc::CIO_Async Io,
          m::ifc::mcu::CIt It>
class Ads1256Reader {
 public:
  Ads1256Reader(Time& time, Io& io, m::ifc::mcu::IPin& cs, It& drdy)
      : time_(time), io_(io), cs_(cs), drdy_(drdy) {
    drdy_.setCallback([&]() {
      if (auto value = adc_ic_.template read<Ads1256::Data>(); value) {
        auto reg = value.value();
        uint32_t reg_raw = reg.value.getRaw();
        if (reg_raw & 0x80'00'00) {
          reg_raw |= 0xFF'00'00'00;
        }
        if (data_.size()) {
          data_[0] = reg_raw;
          data_ = data_.subspan(1);
        } else {
          drdy_.stop();
        }
      }
    });
  }

  bool startRead(std::span<uint32_t> data) {
    data_ = data;
    size_ = data_.size();
    if (!drdy_.start()) return false;
    return true;
  }

  bool readDone() { return !drdy_.running(); }

  std::size_t readed() { return size_ - data_.size(); }

  bool stopRead() { return drdy_.stop(); }

 private:
  Time& time_;
  Io& io_;
  m::ifc::mcu::IPin& cs_;
  It& drdy_;

  Ads1256Ic<Us, Time, Io> adc_ic_{time_, io_, cs_, Us{20}};

  std::span<uint32_t> data_;
  std::size_t size_;
};

}  // namespace m::ic

#endif  // ADS1256_HPP