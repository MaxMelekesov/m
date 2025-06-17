#pragma once

#include <Reg.hpp>
#include <cstdint>

namespace m::tests {

// Test basic field definitions
struct TestField8 : public BitField<8, TestField8> {};
struct TestField16 : public BitField<16, TestField16> {};
struct TestField4 : public BitField<4, TestField4> {};
struct TestField1 : public BitField<1, TestField1> {};

// Valid register definitions
struct ValidReg32Info {
    using BitFieldsOrder = RegBitMap<uint32_t, TestField8, TestField16, TestField4, DummyField<4>>;
};

struct ValidReg16Info {
    using BitFieldsOrder = RegBitMap<uint16_t, TestField8, TestField8>;
};

struct ValidReg8Info {
    using BitFieldsOrder = RegBitMap<uint8_t, TestField4, TestField4>;
};

// Edge case: Single bit field
struct SingleBitInfo {
    using BitFieldsOrder = RegBitMap<uint8_t, TestField1, DummyField<7>>;
};

// Edge case: All dummy fields
struct AllDummyInfo {
    using BitFieldsOrder = RegBitMap<uint32_t, DummyField<16>, DummyField<16>>;
};

// Edge case: Maximum size field (64 bit)
struct MaxField64 : public BitField<64, MaxField64> {};
struct MaxSize64Info {
    using BitFieldsOrder = RegBitMap<uint64_t, MaxField64>;
};

// Edge case: Maximum size field (32 bit)
struct MaxField32 : public BitField<32, MaxField32> {};
struct MaxSize32Info {
    using BitFieldsOrder = RegBitMap<uint32_t, MaxField32>;
};

// Compile-time tests
namespace compile_time_tests {

// Test 1: Basic functionality
constexpr bool test_basic_operations() {
    using TestReg = Reg<ValidReg32Info>;
    TestReg reg;
    
    // Test set/get
    reg.set<TestField8>(0xFF);
    if (reg.get<TestField8>() != 0xFF) return false;
    
    reg.set<TestField16>(0x1234);
    if (reg.get<TestField16>() != 0x1234) return false;
    
    reg.set<TestField4>(0xF);
    if (reg.get<TestField4>() != 0xF) return false;
    
    return true;
}

// Test 2: Value overflow handling
constexpr bool test_value_overflow() {
    using TestReg = Reg<ValidReg32Info>;
    TestReg reg;
    
    // Test overflow - should be masked
    reg.set<TestField8>(0x1FF); // 9 bits, should be masked to 8
    if (reg.get<TestField8>() != 0xFF) return false;
    
    reg.set<TestField4>(0x1F); // 5 bits, should be masked to 4
    if (reg.get<TestField4>() != 0xF) return false;
    
    return true;
}

// Test 3: Field isolation
constexpr bool test_field_isolation() {
    using TestReg = Reg<ValidReg32Info>;
    TestReg reg;
    
    // Set all fields to different values
    reg.set<TestField8>(0xAA);
    reg.set<TestField16>(0x5555);
    reg.set<TestField4>(0xC);
    
    // Verify they don't interfere
    if (reg.get<TestField8>() != 0xAA) return false;
    if (reg.get<TestField16>() != 0x5555) return false;
    if (reg.get<TestField4>() != 0xC) return false;
    
    return true;
}

// Test 4: Raw value operations
constexpr bool test_raw_operations() {
    using TestReg = Reg<ValidReg32Info>;
    TestReg reg;
    
    // Test raw set/get
    reg.setRaw(0x12345678);
    auto raw = reg.getRaw();
    
    // Should mask dummy fields
    if (raw != (0x12345678 & ValidReg32Info::BitFieldsOrder::non_dummy_mask_)) {
        return false;
    }
    
    return true;
}

// Test 5: Constructor with value
constexpr bool test_constructor() {
    using TestReg = Reg<ValidReg32Info>;
    TestReg reg(0xFFFFFFFF);
    
    // Should mask dummy fields
    auto raw = reg.getRaw();
    if (raw != ValidReg32Info::BitFieldsOrder::non_dummy_mask_) {
        return false;
    }
    
    return true;
}

// Test 6: Single bit operations
constexpr bool test_single_bit() {
    using TestReg = Reg<SingleBitInfo>;
    TestReg reg;
    
    reg.set<TestField1>(1);
    if (reg.get<TestField1>() != 1) return false;
    
    reg.set<TestField1>(0);
    if (reg.get<TestField1>() != 0) return false;
    
    // Test overflow
    reg.set<TestField1>(5); // Should be masked to 1
    if (reg.get<TestField1>() != 1) return false;
    
    return true;
}

// Test 7: Maximum size fields
constexpr bool test_max_size() {
    // Test 32-bit field
    {
        using TestReg = Reg<MaxSize32Info>;
        TestReg reg;
        
        reg.set<MaxField32>(0xFFFFFFFFU);
        if (reg.get<MaxField32>() != 0xFFFFFFFFU) return false;
    }
    
    // Test 64-bit field
    {
        using TestReg = Reg<MaxSize64Info>;
        TestReg reg;
        
        reg.set<MaxField64>(0xFFFFFFFFFFFFFFFFULL);
        if (reg.get<MaxField64>() != 0xFFFFFFFFFFFFFFFFULL) return false;
    }
    
    return true;
}

// Test 8: All dummy fields register
constexpr bool test_all_dummy() {
    using TestReg = Reg<AllDummyInfo>;
    TestReg reg;
    
    reg.setRaw(0xFFFFFFFF);
    if (reg.getRaw() != 0) return false; // Should be 0 due to all dummy
    
    return true;
}

// Execute all tests
constexpr bool run_all_tests() {
    return test_basic_operations() &&
           test_value_overflow() &&
           test_field_isolation() &&
           test_raw_operations() &&
           test_constructor() &&
           test_single_bit() &&
           test_max_size() &&
           test_all_dummy();
}

// Execute tests individually to find issues
static_assert(test_basic_operations(), "Basic operations test failed");
static_assert(test_value_overflow(), "Value overflow test failed");
static_assert(test_field_isolation(), "Field isolation test failed");
static_assert(test_raw_operations(), "Raw operations test failed");
static_assert(test_constructor(), "Constructor test failed");
static_assert(test_single_bit(), "Single bit test failed");
static_assert(test_max_size(), "Max size test failed");
static_assert(test_all_dummy(), "All dummy test failed");

// Compile-time assertion
static_assert(run_all_tests(), "Reg compile-time tests failed");

} // namespace compile_time_tests

// Error case tests (should not compile)
namespace error_tests {

// Error 1: Invalid field size (0)
// struct InvalidField0 : public BitField<0, InvalidField0> {}; // Should not compile

// Error 2: Invalid field size (>64)
// struct InvalidField65 : public BitField<65, InvalidField65> {}; // Should not compile

// Error 4: No BitFieldsOrder
// struct NoBitFieldsOrder {};
// using InvalidReg = Reg<NoBitFieldsOrder>; // Should not compile

// Error 5: Empty field list
// struct EmptyFieldsInfo {
//     using BitFieldsOrder = RegBitMap<uint32_t>; // No fields
// };

// Error 6: Non-integral storage
// struct NonIntegralInfo {
//     using BitFieldsOrder = RegBitMap<float, TestField8, DummyField<24>>; // Should not compile
// };


} // namespace error_tests

} // namespace m::tests
