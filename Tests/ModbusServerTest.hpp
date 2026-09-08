/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2026 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef MODBUS_SERVER_TEST_HPP
#define MODBUS_SERVER_TEST_HPP

#include <ModbusServer.hpp>

#include <array>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include <span>
#include <tuple>
#include <type_traits>

namespace m::tsts {

// ===========================================================================
// Compile-time checks — no runtime cost, failures are compile errors.
// ===========================================================================

// ---- ModbusType sizing ----------------------------------------------------
static_assert(ModbusType<uint16_t>::paddedSize == 2);
static_assert(ModbusType<uint16_t>::regCount == 1);
static_assert(ModbusType<uint32_t>::regCount == 2);
static_assert(ModbusType<int32_t>::regCount == 2);
static_assert(ModbusType<float>::regCount == 2);
static_assert(ModbusType<double>::regCount == 4);

// ---- Access predicates ----------------------------------------------------
static_assert(detail::isHolding(ModbusAccess::HoldingRO));
static_assert(detail::isHolding(ModbusAccess::HoldingWO));
static_assert(detail::isHolding(ModbusAccess::HoldingRW));
static_assert(!detail::isHolding(ModbusAccess::CoilRO));
static_assert(detail::isCoil(ModbusAccess::CoilRO));
static_assert(detail::isCoil(ModbusAccess::CoilRW));
static_assert(!detail::isCoil(ModbusAccess::DiscreteRO));
static_assert(detail::isDiscrete(ModbusAccess::DiscreteRO));
static_assert(detail::isReadable(ModbusAccess::HoldingRO));
static_assert(detail::isReadable(ModbusAccess::HoldingRW));
static_assert(!detail::isReadable(ModbusAccess::HoldingWO));
static_assert(detail::isReadable(ModbusAccess::CoilRO));
static_assert(detail::isReadable(ModbusAccess::CoilRW));
static_assert(detail::isReadable(ModbusAccess::DiscreteRO));
static_assert(detail::isWritable(ModbusAccess::HoldingWO));
static_assert(detail::isWritable(ModbusAccess::HoldingRW));
static_assert(!detail::isWritable(ModbusAccess::HoldingRO));
static_assert(detail::isWritable(ModbusAccess::CoilRW));
static_assert(!detail::isWritable(ModbusAccess::CoilRO));

// ---- Key traits & concepts ------------------------------------------------
struct K_H : ModbusReg<uint16_t, ModbusAccess::HoldingRW, 0x0000> {};
static_assert(K_H::address == 0x0000);
static_assert(K_H::isHolding && !K_H::isCoil && !K_H::isDiscrete);
static_assert(std::is_same_v<K_H::ValueType, uint16_t>);
static_assert(CModbusKey<K_H>);

struct K_U32 : ModbusReg<uint32_t, ModbusAccess::HoldingRW, 0x0002> {};
static_assert(K_U32::Type::regCount == 2);
static_assert(CModbusKey<K_U32>);

struct K_C : ModbusCoil<ModbusAccess::CoilRW, 0x0000> {};
static_assert(K_C::isCoil && !K_C::isHolding && !K_C::isDiscrete);
static_assert(CModbusKey<K_C>);

struct K_D : ModbusCoil<ModbusAccess::DiscreteRO, 0x0000> {};
static_assert(K_D::isDiscrete && !K_D::isCoil && !K_D::isHolding);
static_assert(CModbusKey<K_D>);

// ---- ModbusType value types ----------------------------------------------
static_assert(std::is_same_v<ModbusType<float>::ValueType, float>);
static_assert(std::is_same_v<ModbusType<double>::ValueType, double>);
static_assert(std::is_same_v<ModbusType<int32_t>::ValueType, int32_t>);

// ---- ModbusKey tag type ---------------------------------------------------
static_assert(std::is_same_v<ModbusKey<K_H>::KeyType, K_H>);
static_assert(std::is_same_v<ModbusKey<K_U32>::KeyType, K_U32>);

// ---- Compile-time map validation ------------------------------------------
// Positive: non-overlapping holding keys.
static_assert(detail::allAddressesUnique<std::tuple<K_H, K_U32>>());
static_assert(detail::allTypesUnique<std::tuple<K_H, K_U32, K_C, K_D>>());
// Positive: adjacent multi-register keys (u32 @2..3 + u16 @4) do not overlap.
struct Adj_A : ModbusReg<uint32_t, ModbusAccess::HoldingRW, 0x0002> {};
struct Adj_B : ModbusReg<uint16_t, ModbusAccess::HoldingRW, 0x0004> {};
static_assert(detail::allAddressesUnique<std::tuple<Adj_A, Adj_B>>());
// Positive: coil and discrete live in separate Modbus address spaces —
//           the same numeric address is allowed.
static_assert(detail::allAddressesUnique<std::tuple<K_C, K_D>>());
// Positive: two coils at different addresses.
struct Coil_At_1 : ModbusCoil<ModbusAccess::CoilRW, 0x0001> {};
static_assert(detail::allAddressesUnique<std::tuple<K_C, Coil_At_1>>());

// Negative: holding range overlap — u32 @0x0000 (words 0..1) vs u16 @0x0001.
struct OvA : ModbusReg<uint32_t, ModbusAccess::HoldingRW, 0x0000> {};
struct OvB : ModbusReg<uint16_t, ModbusAccess::HoldingRW, 0x0001> {};
static_assert(!detail::allAddressesUnique<std::tuple<OvA, OvB>>());
// Negative: containment — u32 @0x0000 contains u16 @0x0000.
struct OvC : ModbusReg<uint16_t, ModbusAccess::HoldingRW, 0x0000> {};
struct OvD : ModbusReg<uint32_t, ModbusAccess::HoldingRW, 0x0000> {};
static_assert(!detail::allAddressesUnique<std::tuple<OvC, OvD>>());
// Negative: two coils at the same address.
struct CoilA : ModbusCoil<ModbusAccess::CoilRW, 0x0005> {};
struct CoilB : ModbusCoil<ModbusAccess::CoilRO, 0x0005> {};
static_assert(!detail::allAddressesUnique<std::tuple<CoilA, CoilB>>());
// Negative: duplicate key type in the tuple.
static_assert(!detail::allTypesUnique<std::tuple<K_H, K_H>>());
static_assert(!detail::allTypesUnique<std::tuple<K_C, K_C>>());

// ---- Feature presence (Has_* gating) --------------------------------------
static_assert(detail::hasAnyHolding<K_H>);
static_assert(detail::hasAnyHolding<K_U32>);
static_assert(!detail::hasAnyHolding<K_C, K_D>);
static_assert(detail::hasAnyCoil<K_C>);
static_assert(!detail::hasAnyCoil<K_H, K_D>);
static_assert(detail::hasAnyDiscrete<K_D>);
static_assert(!detail::hasAnyDiscrete<K_H, K_C>);

// ---- Key categories & access on all six kinds -----------------------------
struct K_WO : ModbusReg<uint16_t, ModbusAccess::HoldingWO, 0x0010> {};
static_assert(K_WO::access == ModbusAccess::HoldingWO);
static_assert(K_WO::isHolding && !K_WO::isCoil && !K_WO::isDiscrete);
static_assert(detail::isWritable(K_WO::access));
static_assert(!detail::isReadable(K_WO::access));
static_assert(CModbusKey<K_WO>);

struct K_CRO : ModbusCoil<ModbusAccess::CoilRO, 0x0000> {};
static_assert(K_CRO::isCoil && !K_CRO::isDiscrete);
static_assert(detail::isReadable(K_CRO::access));
static_assert(!detail::isWritable(K_CRO::access));

// ---- Address-space edge ----------------------------------------------------
struct K_Edge : ModbusReg<uint16_t, ModbusAccess::HoldingRW, 0xFFFF> {};
static_assert(K_Edge::address == 0xFFFF);
static_assert(detail::allAddressesUnique<std::tuple<K_Edge>>());

struct MapCt { using Keys = std::tuple<K_H, K_U32, K_C, K_D>; };
static_assert(CModbusRegInfo<MapCt>);

// ---- Concept negatives ----------------------------------------------------
static_assert(!CModbusKey<int>);
static_assert(!CModbusRegInfo<int>);
struct BadRegInfo { using Keys = int; };
static_assert(!CModbusRegInfo<BadRegInfo>);

// ---- CRC-16 known vectors ------------------------------------------------
struct DummyTime {
  using Unit = Us<uint32_t>;
  void delay(Us<uint32_t>) {}
  Us<uint32_t> now() { return Us<uint32_t>{0}; }
  Us<uint32_t> diff(Us<uint32_t>) { return Us<uint32_t>{0}; }
};
struct DummyHandler {};
using CtServer = ModbusServer<DummyTime, detail::NoOpCb, detail::NoOpCb,
                              DummyHandler>;

constexpr std::array<uint8_t, 6> kCrcVec1{0x01, 0x03, 0x00, 0x00, 0x00, 0x01};
constexpr std::array<uint8_t, 6> kCrcVec2{0x01, 0x03, 0x00, 0x00, 0x00, 0x02};
// Wire order is low byte first: 0x0A84 -> bytes 84 0A; 0x0BC4 -> bytes C4 0B.
static_assert(CtServer::crc16(
                  std::span<const uint8_t>(kCrcVec1.data(), kCrcVec1.size())) ==
              0x0A84);
static_assert(CtServer::crc16(
                  std::span<const uint8_t>(kCrcVec2.data(), kCrcVec2.size())) ==
              0x0BC4);

// ===========================================================================
// Runtime tests (host) — dispatch-level, no hardware / no coroutine needed.
// ===========================================================================

struct TestMap {
  struct Dev_Id : ModbusReg<uint16_t, ModbusAccess::HoldingRO, 0x0000> {};
  struct Ctrl : ModbusReg<uint16_t, ModbusAccess::HoldingRW, 0x0001> {};
  struct Sp1 : ModbusReg<uint32_t, ModbusAccess::HoldingRW, 0x0002> {};
  struct Sp2 : ModbusReg<uint32_t, ModbusAccess::HoldingRW, 0x0004> {};
  struct Mode : ModbusReg<uint16_t, ModbusAccess::HoldingRW, 0x0006> {};
  struct Cmd : ModbusReg<uint16_t, ModbusAccess::HoldingWO, 0x0007> {};
  struct Coil : ModbusCoil<ModbusAccess::CoilRW, 0x0000> {};
  struct Disc : ModbusCoil<ModbusAccess::DiscreteRO, 0x0000> {};
  using Keys = std::tuple<Dev_Id, Ctrl, Sp1, Sp2, Mode, Cmd, Coil, Disc>;
};

struct TestState {
  uint16_t dev_id = 0x0001;
  uint16_t ctrl = 0;
  uint16_t mode = 0;
  uint16_t cmd = 0;
  uint32_t sp1 = 0;
  uint32_t sp2 = 0;
  bool coil = false;
  bool disc = true;
};

struct TestHandler : ModbusHandler<TestMap, TestHandler> {
  using Base = ModbusHandler<TestMap, TestHandler>;
  TestHandler(uint8_t addr, TestState& st) : Base(addr), st_(st) {}

  uint16_t onRead(ModbusKey<TestMap::Dev_Id>) const { return st_.dev_id; }
  uint16_t onRead(ModbusKey<TestMap::Ctrl>) const { return st_.ctrl; }
  uint32_t onRead(ModbusKey<TestMap::Sp1>) const { return st_.sp1; }
  uint32_t onRead(ModbusKey<TestMap::Sp2>) const { return st_.sp2; }
  uint16_t onRead(ModbusKey<TestMap::Mode>) const { return st_.mode; }
  bool onRead(ModbusKey<TestMap::Coil>) const { return st_.coil; }
  bool onRead(ModbusKey<TestMap::Disc>) const { return st_.disc; }

  void onWrite(ModbusKey<TestMap::Ctrl>, uint16_t v) { st_.ctrl = v; }
  void onWrite(ModbusKey<TestMap::Sp1>, uint32_t v) { st_.sp1 = v; }
  void onWrite(ModbusKey<TestMap::Sp2>, uint32_t v) { st_.sp2 = v; }
  void onWrite(ModbusKey<TestMap::Mode>, uint16_t v) { st_.mode = v; }
  void onWrite(ModbusKey<TestMap::Cmd>, uint16_t v) { st_.cmd = v; }
  void onWrite(ModbusKey<TestMap::Coil>, bool v) { st_.coil = v; }

  TestState& st_;
};

namespace detail_test {
inline uint16_t be16(const uint8_t* p) {
  return static_cast<uint16_t>((p[0] << 8) | p[1]);
}
// Low word at base address, then high word (младшее слово по базовому адресу).
inline uint32_t val32(const uint8_t* p) {
  return (static_cast<uint32_t>(be16(p + 2)) << 16) | be16(p);
}
inline void put32(uint8_t* p, uint32_t v) {
  p[0] = static_cast<uint8_t>(v >> 8);
  p[1] = static_cast<uint8_t>(v);
  p[2] = static_cast<uint8_t>(v >> 24);
  p[3] = static_cast<uint8_t>(v >> 16);
}
}  // namespace detail_test

inline bool modbusServerDispatchTest() {
  using namespace detail_test;
  int fails = 0;
#define T_CHECK(cond)                                                \
  do {                                                               \
    if (!(cond)) {                                                   \
      ++fails;                                                       \
      std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);    \
    }                                                                \
  } while (0)

  TestState st;
  TestHandler h{0x0A, st};
  std::array<uint8_t, 256> resp{};
  st.dev_id = 0x0001;
  st.ctrl = 0xABCD;
  st.sp1 = 0x11223344u;
  st.sp2 = 0x55667788u;
  st.mode = 3;
  st.coil = true;
  st.disc = true;

  {  // 1. single u16 read (Dev_Id @0)
    uint8_t req[] = {0, 0, 0, 1};
    auto r = h.dispatch(0x03, req, resp);
    T_CHECK(r.has_value() && resp[0] == 2 && be16(&resp[1]) == 0x0001);
  }
  {  // 2. whole u32 read (Sp1 @2..3)
    uint8_t req[] = {0, 2, 0, 2};
    auto r = h.dispatch(0x03, req, resp);
    T_CHECK(r.has_value() && resp[0] == 4 && val32(&resp[1]) == 0x11223344u);
  }
  {  // 3. block read 0..8 (keys + WO Cmd @7 => 0)
    uint8_t req[] = {0, 0, 0, 8};
    auto r = h.dispatch(0x03, req, resp);
    T_CHECK(r.has_value() && resp[0] == 16);
    T_CHECK(be16(&resp[1]) == 0x0001);        // Dev_Id
    T_CHECK(be16(&resp[3]) == 0xABCD);        // Ctrl
    T_CHECK(val32(&resp[5]) == 0x11223344u);  // Sp1
    T_CHECK(val32(&resp[9]) == 0x55667788u);  // Sp2
    T_CHECK(be16(&resp[13]) == 3);            // Mode
    T_CHECK(be16(&resp[15]) == 0);            // Cmd (WO) -> 0
  }
  {  // 4. block read 0..9 — word 8 is a gap => 0
    uint8_t req[] = {0, 0, 0, 9};
    auto r = h.dispatch(0x03, req, resp);
    T_CHECK(r.has_value() && resp[0] == 18 && be16(&resp[17]) == 0);
  }
  {  // 5. partial (cut) u32 read -> IllegalDataValue
    uint8_t req[] = {0, 3, 0, 1};
    auto r = h.dispatch(0x03, req, resp);
    T_CHECK(!r.has_value() &&
            r.error() == ModbusRtuError::IllegalDataValue);
  }
  {  // 6. FC06 single write Ctrl
    uint8_t req[] = {0, 1, 0x12, 0x34};
    auto r = h.dispatch(0x06, req, resp);
    T_CHECK(r.has_value() && st.ctrl == 0x1234);
  }
  {  // 7. FC06 to a u32 key -> IllegalDataValue
    uint8_t req[] = {0, 2, 0x12, 0x34};
    auto r = h.dispatch(0x06, req, resp);
    T_CHECK(!r.has_value() &&
            r.error() == ModbusRtuError::IllegalDataValue);
  }
  {  // 8. FC06 to RO key -> IllegalFunction
    uint8_t req[] = {0, 0, 0x12, 0x34};
    auto r = h.dispatch(0x06, req, resp);
    T_CHECK(!r.has_value() && r.error() == ModbusRtuError::IllegalFunction);
  }
  {  // 9. FC06 WO command (Cmd @7)
    uint8_t req[] = {0, 7, 0x00, 0x01};
    auto r = h.dispatch(0x06, req, resp);
    T_CHECK(r.has_value() && st.cmd == 1);
  }
  {  // 10. FC16 block write Ctrl(1)+Sp1(2..3)
    uint8_t req[] = {0, 1, 0, 3, 6, 0xAA, 0xBB, 0, 0, 0, 0};
    put32(&req[7], 0x22114433u);
    auto r = h.dispatch(0x10, req, resp);
    T_CHECK(r.has_value());
    T_CHECK(st.ctrl == 0xAABB);
    T_CHECK(st.sp1 == 0x22114433u);
  }
  {  // 11. FC16 write whole Sp2
    uint8_t req[] = {0, 4, 0, 2, 4, 0, 0, 0, 0};
    put32(&req[5], 0x77665544u);
    auto r = h.dispatch(0x10, req, resp);
    T_CHECK(r.has_value() && st.sp2 == 0x77665544u);
  }
  {  // 12. FC16 partial (cut u32) -> error, nothing applied
    const uint16_t before = st.ctrl;
    uint8_t req[] = {0, 1, 0, 2, 4, 1, 2, 3, 4, 5, 6};
    auto r = h.dispatch(0x10, req, resp);
    T_CHECK(!r.has_value() &&
            r.error() == ModbusRtuError::IllegalDataValue);
    T_CHECK(st.ctrl == before);
  }
  {  // 13. FC16 with gap -> IllegalDataAddress, nothing applied
    uint8_t req[] = {0, 8, 0, 2, 4, 0, 1, 0, 0};
    auto r = h.dispatch(0x10, req, resp);
    T_CHECK(!r.has_value() &&
            r.error() == ModbusRtuError::IllegalDataAddress);
    T_CHECK(st.mode == 3);
  }
  {  // 14. FC01 read coils
    uint8_t req[] = {0, 0, 0, 2};
    auto r = h.dispatch(0x01, req, resp);
    T_CHECK(r.has_value() && resp[0] == 1 && (resp[1] & 0x01) == 0x01);
  }
  {  // 15. FC05 write single coil
    uint8_t req[] = {0, 0, 0xFF, 0x00};
    auto r = h.dispatch(0x05, req, resp);
    T_CHECK(r.has_value() && st.coil == true);
  }
  {  // 16. FC02 read discrete inputs
    uint8_t req[] = {0, 0, 0, 1};
    auto r = h.dispatch(0x02, req, resp);
    T_CHECK(r.has_value() && resp[0] == 1 && (resp[1] & 0x01) == 0x01);
  }

#undef T_CHECK
  return fails == 0;
}

// ===========================================================================
// Runtime transport test — full ADU through ModbusServer::coroRun().
// Covers valid and invalid packets, CRC, addressing, broadcast, multi-address.
// ===========================================================================

struct FakeDl : m::ifc::IDataLink {
  explicit FakeDl(std::span<uint8_t> p) : packet(p) {}

  std::span<uint8_t> packet{};
  std::span<const uint8_t> tx_out{};

  bool startReceive(std::span<uint8_t>) override { return true; }
  std::optional<std::span<uint8_t>> getPacket() override {
    if (packet.size() != 0) {
      auto p = packet;
      packet = {};
      return p;
    }
    return std::nullopt;
  }
  bool startTransmit(std::span<const uint8_t> b) override {
    tx_out = b;
    return true;
  }
  std::optional<bool> transmitDone() override { return true; }
  bool stopReceive() override { return true; }
  bool stopTransmit() override { return true; }
  bool error() override { return false; }
};

struct FakeTime : m::ifc::ITime<Us<uint32_t>> {
  void delay(Us<uint32_t>) override {}
  Us<uint32_t> now() override { return Us<uint32_t>{0}; }
  Us<uint32_t> diff(Us<uint32_t>) override { return Us<uint32_t>{0}; }
};

template <typename S>
m::Task<void> runOnce(S& s, bool& out) {
  out = co_await s.coroRun();
  co_return;
}

// Builds a full RTU frame: [addr, cmd, payload..., crc_lo, crc_hi].
inline std::span<uint8_t> makeFrame(std::array<uint8_t, 256>& buf,
                                    std::initializer_list<uint8_t> hdr) {
  std::size_t n = 0;
  for (uint8_t b : hdr) buf[n++] = b;
  const uint16_t crc =
      CtServer::crc16(std::span<const uint8_t>(buf.data(), n));
  buf[n] = static_cast<uint8_t>(crc);
  buf[n + 1] = static_cast<uint8_t>(crc >> 8);
  return std::span<uint8_t>(buf.data(), n + 2);
}

// Runs one ADU through coroRun(); copies the response bytes into `out`.
template <typename... Hs>
bool runPacket(std::span<uint8_t> frame, std::span<uint8_t> out,
               std::size_t& out_size, Hs&... handlers) {
  using Server = ModbusServer<FakeTime, detail::NoOpCb, detail::NoOpCb, Hs...>;
  std::array<uint8_t, 256> rx{}, tx{};
  FakeTime ft;
  FakeDl dl{frame};
  Server server{dl, ft, {Us<uint32_t>{0}}, rx, tx,
                detail::NoOpCb{}, detail::NoOpCb{}, handlers...};

  bool ok = false;
  auto task = runOnce(server, ok);
  for (int i = 0; i < 1000 && !ok; ++i)
    CoroScheduler::getInstance().handle();

  out_size = dl.tx_out.size();
  const std::size_t n = out_size < out.size() ? out_size : out.size();
  for (std::size_t i = 0; i < n; ++i) out[i] = dl.tx_out[i];
  return ok;
}

inline bool modbusServerTransportTest() {
  int fails = 0;
#define T_CHECK(cond)                                                \
  do {                                                               \
    if (!(cond)) {                                                   \
      ++fails;                                                       \
      std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);    \
    }                                                                \
  } while (0)

  TestState stA, stB;
  stA.dev_id = 0x1234;
  stA.ctrl = 0xABCD;
  stA.coil = true;
  stA.disc = true;
  stB.dev_id = 0x5678;
  stB.ctrl = 0x1111;
  TestHandler hA{0x0A, stA};
  TestHandler hB{0x0B, stB};

  std::array<uint8_t, 256> fbuf{}, obuf{};
  std::size_t osz = 0;
  bool ok = false;

  {  // 1. valid FC03 read Dev_Id(0)+Ctrl(1)
    ok = runPacket(makeFrame(fbuf, {0x0A, 0x03, 0x00, 0x00, 0x00, 0x02}), obuf,
                   osz, hA);
    T_CHECK(ok && osz == 9);
    T_CHECK(obuf[0] == 0x0A && obuf[1] == 0x03 && obuf[2] == 4);
    T_CHECK(obuf[3] == 0x12 && obuf[4] == 0x34);
    T_CHECK(obuf[5] == 0xAB && obuf[6] == 0xCD);
    const uint16_t rcrc =
        CtServer::crc16(std::span<const uint8_t>(obuf.data(), 7));
    T_CHECK(obuf[7] == static_cast<uint8_t>(rcrc) &&
            obuf[8] == static_cast<uint8_t>(rcrc >> 8));
  }
  {  // 2. valid FC06 write Ctrl
    ok = runPacket(makeFrame(fbuf, {0x0A, 0x06, 0x00, 0x01, 0x12, 0x34}), obuf,
                   osz, hA);
    T_CHECK(ok && osz == 8 && stA.ctrl == 0x1234);
    T_CHECK(obuf[0] == 0x0A && obuf[1] == 0x06);
  }
  {  // 3. valid FC01 read coils
    ok = runPacket(makeFrame(fbuf, {0x0A, 0x01, 0x00, 0x00, 0x00, 0x01}), obuf,
                   osz, hA);
    T_CHECK(ok && osz == 6 && obuf[2] == 1 && (obuf[3] & 0x01) == 0x01);
  }
  {  // 4. valid FC02 read discrete inputs
    ok = runPacket(makeFrame(fbuf, {0x0A, 0x02, 0x00, 0x00, 0x00, 0x01}), obuf,
                   osz, hA);
    T_CHECK(ok && osz == 6 && obuf[2] == 1 && (obuf[3] & 0x01) == 0x01);
  }
  {  // 5. bad CRC -> silence (no response)
    auto sp = makeFrame(fbuf, {0x0A, 0x03, 0x00, 0x00, 0x00, 0x02});
    sp[sp.size() - 1] ^= 0xFF;
    ok = runPacket(sp, obuf, osz, hA);
    T_CHECK(ok && osz == 0);
  }
  {  // 6. wrong slave address -> silence
    ok = runPacket(makeFrame(fbuf, {0x0F, 0x03, 0x00, 0x00, 0x00, 0x02}), obuf,
                   osz, hA);
    T_CHECK(ok && osz == 0);
  }
  {  // 7. unknown function code -> IllegalFunction
    ok = runPacket(makeFrame(fbuf, {0x0A, 0x2B, 0x00, 0x00, 0x00, 0x02}), obuf,
                   osz, hA);
    T_CHECK(ok && osz == 5 && obuf[1] == 0xAB && obuf[2] == 0x01);
  }
  {  // 8. FC03 num=0 -> IllegalDataValue
    ok = runPacket(makeFrame(fbuf, {0x0A, 0x03, 0x00, 0x00, 0x00, 0x00}), obuf,
                   osz, hA);
    T_CHECK(ok && osz == 5 && obuf[1] == 0x83 && obuf[2] == 0x03);
  }
  {  // 9. FC03 num=126 (>125) -> IllegalDataValue
    ok = runPacket(makeFrame(fbuf, {0x0A, 0x03, 0x00, 0x00, 0x00, 0x7E}), obuf,
                   osz, hA);
    T_CHECK(ok && osz == 5 && obuf[1] == 0x83 && obuf[2] == 0x03);
  }
  {  // 10. FC03 start overflow -> IllegalDataAddress
    ok = runPacket(makeFrame(fbuf, {0x0A, 0x03, 0xFF, 0xFF, 0x00, 0x01}), obuf,
                   osz, hA);
    T_CHECK(ok && osz == 5 && obuf[1] == 0x83 && obuf[2] == 0x02);
  }
  {  // 11. FC03 partial (cut u32) -> IllegalDataValue
    ok = runPacket(makeFrame(fbuf, {0x0A, 0x03, 0x00, 0x03, 0x00, 0x01}), obuf,
                   osz, hA);
    T_CHECK(ok && osz == 5 && obuf[1] == 0x83 && obuf[2] == 0x03);
  }
  {  // 12. FC06 to u32 key -> IllegalDataValue
    ok = runPacket(makeFrame(fbuf, {0x0A, 0x06, 0x00, 0x02, 0x12, 0x34}), obuf,
                   osz, hA);
    T_CHECK(ok && osz == 5 && obuf[1] == 0x86 && obuf[2] == 0x03);
  }
  {  // 13. FC06 to RO key -> IllegalFunction
    ok = runPacket(makeFrame(fbuf, {0x0A, 0x06, 0x00, 0x00, 0x12, 0x34}), obuf,
                   osz, hA);
    T_CHECK(ok && osz == 5 && obuf[1] == 0x86 && obuf[2] == 0x01);
  }
  {  // 14. FC06 to unmapped addr -> IllegalDataAddress
    ok = runPacket(makeFrame(fbuf, {0x0A, 0x06, 0x00, 0x7F, 0x00, 0x00}), obuf,
                   osz, hA);
    T_CHECK(ok && osz == 5 && obuf[1] == 0x86 && obuf[2] == 0x02);
  }
  {  // 15. valid FC16 block write Ctrl(1)+Sp1(2..3)
    ok = runPacket(
        makeFrame(fbuf, {0x0A, 0x10, 0x00, 0x01, 0x00, 0x03, 0x06,
                         0xAA, 0xBB, 0x44, 0x33, 0x22, 0x11}),
        obuf, osz, hA);
    T_CHECK(ok && osz == 8 && stA.ctrl == 0xAABB && stA.sp1 == 0x22114433u);
  }
  {  // 16. FC16 wrong byte count -> IllegalDataValue
    ok = runPacket(
        makeFrame(fbuf, {0x0A, 0x10, 0x00, 0x01, 0x00, 0x02, 0x05,
                         0xAA, 0xBB, 0xCC, 0xDD}),
        obuf, osz, hA);
    T_CHECK(ok && osz == 5 && obuf[1] == 0x90 && obuf[2] == 0x03);
  }
  {  // 17. FC16 with gap (start=8) -> IllegalDataAddress
    ok = runPacket(
        makeFrame(fbuf, {0x0A, 0x10, 0x00, 0x08, 0x00, 0x02, 0x04,
                         0x00, 0x01, 0x00, 0x02}),
        obuf, osz, hA);
    T_CHECK(ok && osz == 5 && obuf[1] == 0x90 && obuf[2] == 0x02);
  }
  {  // 18. truncated packet (<4 bytes) -> silence
    uint8_t shortf[2] = {0x0A, 0x03};
    ok = runPacket(std::span<uint8_t>(shortf, 2), obuf, osz, hA);
    T_CHECK(ok && osz == 0);
  }
  {  // 19. broadcast FC06 (addr 0) applied to both handlers, no response
    stA.ctrl = 0;
    stB.ctrl = 0;
    ok = runPacket(makeFrame(fbuf, {0x00, 0x06, 0x00, 0x01, 0x00, 0x42}), obuf,
                   osz, hA, hB);
    T_CHECK(ok && osz == 0);
    T_CHECK(stA.ctrl == 0x0042 && stB.ctrl == 0x0042);
  }
  {  // 20. multi-address unicast to 0x0B
    ok = runPacket(makeFrame(fbuf, {0x0B, 0x03, 0x00, 0x00, 0x00, 0x01}), obuf,
                   osz, hA, hB);
    T_CHECK(ok && osz == 7 && obuf[0] == 0x0B);
    T_CHECK(obuf[3] == 0x56 && obuf[4] == 0x78);  // Dev_Id of stB
  }

#undef T_CHECK
  return fails == 0;
}

/// Runs all runtime tests (dispatch + transport) — callable on the target.
/// Returns true when every check passed; use the result as a pass/fail signal
/// (blink an LED / raise an error flag). The compile-time static_asserts above
/// are checked automatically whenever this header is compiled.
inline bool modbusServerRunAllTests() {
  bool ok = true;
  ok = modbusServerDispatchTest() && ok;
  ok = modbusServerTransportTest() && ok;
  return ok;
}

}  // namespace m::tsts

#endif  // MODBUS_SERVER_TEST_HPP
