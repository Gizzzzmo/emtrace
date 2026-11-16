// Tests how pointers to records are serialized, the MGIC record, and the composition of sinks that
// change the encoding of the stream.
#include "cxx_test.hpp"
#include "test_bytes.hpp"

#include <algorithm>
#include <cstdint>
#include <emtrace/emtrace.hpp>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

using cxxtest::Bytes;
using cxxtest::bytes_of;
using cxxtest::concat;
using cxxtest::payload;

namespace {

template <typename S>
auto serialize_pointer(S& sink, std::uintptr_t address) -> void {
    emt::Sink<S&> sink_impl{sink, nullptr, 0};
    (void) emt::Sink<S&>::out_ptr(sink_impl, reinterpret_cast<const void*>(address));
}

} // namespace

CXX_TEST(pointers_are_shifted_and_written_in_little_endian_order) {
    emt::ChangePtr<3, 8, Bytes> sink{};
    serialize_pointer(sink, 0xABCDEF00U);

    CXX_CHECK_EQ(sink.inner, (Bytes{0xEF, 0xCD, 0xAB}));
}

CXX_TEST(change_ptr_selects_width_and_shift) {
    {
        emt::ChangePtr<2, 4, Bytes> sink{};
        serialize_pointer(sink, 0x1234560U);
        CXX_CHECK_EQ(sink.inner, (Bytes{0x56, 0x34}));
    }
    {
        emt::ChangePtr<1, 0, Bytes> sink{};
        serialize_pointer(sink, 0x1FFU);
        CXX_CHECK_EQ(sink.inner, (Bytes{0xFF}));
    }
    {
        emt::ChangePtr<3, 12, Bytes> sink{};
        serialize_pointer(sink, 0x12345000U);
        CXX_CHECK_EQ(sink.inner, (Bytes{0x45, 0x23, 0x01}));
    }
    {
        emt::ChangePtr<sizeof(emt_ptr_t), 0, Bytes> sink{};
        serialize_pointer(sink, 0x0102030405060708ULL);
        CXX_CHECK_EQ(sink.inner, bytes_of(emt_ptr_t{0x0102030405060708ULL}));
    }
}

CXX_TEST(change_ptr_exposes_its_dimensions) {
    using Sink3x10 = emt::Sink<emt::ChangePtr<3, 10, Bytes>&>;
    static_assert(Sink3x10::ptr_width == 3);
    static_assert(Sink3x10::alignment_power == 10);
    static_assert(Sink3x10::encoding_id == EMT_ENCODING_NONE);

    using SinkCobs = emt::Sink<emt::ChangePtr<2, 9, emt::Cobs<Bytes>>&>;
    static_assert(SinkCobs::ptr_width == 2);
    static_assert(SinkCobs::alignment_power == 9);
    static_assert(SinkCobs::encoding_id == EMT_ENCODING_COBS);
}

CXX_TEST(default_vector_and_ostream_sinks_agree_on_the_pointer) {
    Bytes vector;
    serialize_pointer(vector, 0x123456700U);

    std::ostringstream stream;
    std::ostream& stream_ref = stream;
    {
        emt::Sink<std::ostream&> sink_impl{stream_ref, nullptr, 0};
        (void) emt::Sink<std::ostream&>::out_ptr(
            sink_impl, reinterpret_cast<const void*>(0x123456700U)
        );
    }
    const std::string written = stream.str();

    CXX_CHECK_EQ(vector, (Bytes{0x67, 0x45, 0x23}));
    CXX_CHECK_EQ(Bytes(written.begin(), written.end()), vector);
}

CXX_TEST(cobs_sink_frames_the_whole_record) {
    emt::Cobs<Bytes> sink{};
    MTRACE("{}", 0, emt::opt_sink(sink));

    // The integer is made of four zeros, but the encoded record contains a single one: the
    // terminator. Pointer (3 bytes) and integer (4 bytes) become 7 + 1 bytes, plus the terminator.
    CXX_CHECK_EQ(std::count(sink.inner.begin(), sink.inner.end(), 0), 1);
    CXX_CHECK_EQ(sink.inner.back(), 0);
    CXX_CHECK_EQ(sink.inner.size(), 9U);
}

CXX_TEST(magic_record_describes_the_stream) {
    const emt_magic_t record = emt::make_magic_record(
        emt::MagicRecordConfig{
            .encoding = EMT_ENCODING_COBS,
            .ptr_width = 2,
            .alignment_power = 11,
        }
    );

    CXX_CHECK_EQ(record.framing[0], 'E');
    CXX_CHECK_EQ(record.framing[1], 'M');
    CXX_CHECK_EQ(record.framing[2], 'T');
    CXX_CHECK_EQ(record.framing[10], 'M');
    CXX_CHECK_EQ(record.framing[11], 'G');
    CXX_CHECK_EQ(record.framing[12], 'I');
    CXX_CHECK_EQ(record.framing[13], 'C');
    CXX_CHECK_EQ(record.record_size, sizeof(emt_magic_t));
    CXX_CHECK_EQ(record.meta[32], 2);
    CXX_CHECK_EQ(record.meta[33], 11);
    CXX_CHECK_EQ(record.meta[34], EMT_ENCODING_COBS);
}

CXX_TEST(magic_record_is_derived_from_the_sink) {
    {
        constexpr emt_magic_t record = emt::make_magic_record<emt::StdoutSink>();
        static_assert(record.meta[32] == emt::default_ptr_width);
        static_assert(record.meta[33] == EMT_DEFAULT_ALIGNMENT_POWER);
        static_assert(record.meta[34] == EMT_ENCODING_NONE);
    }
    {
        constexpr emt_magic_t record = emt::make_magic_record<emt::Cobs<>>();
        static_assert(record.meta[32] == emt::default_ptr_width);
        static_assert(record.meta[34] == EMT_ENCODING_COBS);
    }
    {
        constexpr emt_magic_t record = emt::make_magic_record<emt::ChangePtr<2, 10, emt::Cobs<>>>();
        static_assert(record.meta[32] == 2);
        static_assert(record.meta[33] == 10);
        static_assert(record.meta[34] == EMT_ENCODING_COBS);
    }
}

CXX_TEST(magic_records_share_the_constant_that_identifies_them) {
    const emt_magic_t a = emt::make_magic_record();
    const emt_magic_t b =
        emt::make_magic_record(emt::MagicRecordConfig{.encoding = EMT_ENCODING_COBS});

    CXX_CHECK_EQ(Bytes(a.meta, a.meta + 32), Bytes(b.meta, b.meta + 32));
}
