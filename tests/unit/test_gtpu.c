/* SPDX-License-Identifier: MIT */
#include "minitest.h"
#include "upf/gtpu.h"

#include <stdint.h>

/* G-PDU, no optional fields, TEID 0x11223344, 4-byte payload */
static const uint8_t k_gpdu_plain[] = {
    0x30, 0xFF, 0x00, 0x04, 0x11, 0x22, 0x33, 0x44, 0x45, 0x00, 0x00, 0x14,
};

/* G-PDU with E flag and a PDU Session Container (UL, QFI 9), 2-byte payload */
static const uint8_t k_gpdu_qfi[] = {
    0x34, 0xFF, 0x00, 0x0A, /* flags E, type, length = 4 opt + 4 ext + 2 */
    0x00, 0x00, 0x00, 0x01, /* TEID 1 */
    0x00, 0x00, 0x00, 0x85, /* seq, npdu, next = PDU Session Container */
    0x01, 0x10, 0x09, 0x00, /* len=1 (4 octets), UL, QFI=9, next = none */
    0xAB, 0xCD,             /* payload */
};

/* Echo Request with sequence number 0x1234 */
static const uint8_t k_echo_req[] = {
    0x32, 0x01, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x12, 0x34, 0x00, 0x00,
};

TEST(parse_plain_gpdu)
{
    gtpu_hdr_t h;
    ASSERT_EQ(GTPU_OK, gtpu_parse(k_gpdu_plain, sizeof(k_gpdu_plain), &h));
    ASSERT_EQ(GTPU_G_PDU, h.type);
    ASSERT_EQ(0x11223344u, h.teid);
    ASSERT_EQ(8, h.hdr_len);
    ASSERT_EQ(4, h.payload_len);
    ASSERT_TRUE(!h.has_opt);
    ASSERT_TRUE(!h.has_qfi);
}

TEST(parse_gpdu_with_qfi)
{
    gtpu_hdr_t h;
    ASSERT_EQ(GTPU_OK, gtpu_parse(k_gpdu_qfi, sizeof(k_gpdu_qfi), &h));
    ASSERT_TRUE(h.has_opt);
    ASSERT_TRUE(h.has_qfi);
    ASSERT_EQ(9, h.qfi);
    ASSERT_EQ(16, h.hdr_len);
    ASSERT_EQ(2, h.payload_len);
    ASSERT_EQ(0xAB, k_gpdu_qfi[h.hdr_len]);
}

TEST(parse_echo_request)
{
    gtpu_hdr_t h;
    ASSERT_EQ(GTPU_OK, gtpu_parse(k_echo_req, sizeof(k_echo_req), &h));
    ASSERT_EQ(GTPU_ECHO_REQUEST, h.type);
    ASSERT_EQ(0x1234, h.seq);
    ASSERT_EQ(0, h.payload_len);
}

TEST(reject_short_and_bad_headers)
{
    gtpu_hdr_t h;
    uint8_t b[sizeof(k_gpdu_plain)];

    ASSERT_EQ(GTPU_ERR_SHORT, gtpu_parse(k_gpdu_plain, 7, &h));

    memcpy(b, k_gpdu_plain, sizeof(b));
    b[0] = 0x50; /* version 2 */
    ASSERT_EQ(GTPU_ERR_VERSION, gtpu_parse(b, sizeof(b), &h));

    memcpy(b, k_gpdu_plain, sizeof(b));
    b[0] = 0x20; /* PT = 0 */
    ASSERT_EQ(GTPU_ERR_PT, gtpu_parse(b, sizeof(b), &h));

    memcpy(b, k_gpdu_plain, sizeof(b));
    b[3] = 0x10; /* length larger than datagram */
    ASSERT_EQ(GTPU_ERR_LENGTH, gtpu_parse(b, sizeof(b), &h));
}

TEST(reject_bad_extension_chain)
{
    gtpu_hdr_t h;
    uint8_t b[sizeof(k_gpdu_qfi)];

    memcpy(b, k_gpdu_qfi, sizeof(b));
    b[12] = 0x00; /* ext length 0 is illegal */
    ASSERT_EQ(GTPU_ERR_EXT, gtpu_parse(b, sizeof(b), &h));

    memcpy(b, k_gpdu_qfi, sizeof(b));
    b[12] = 0x05; /* ext header runs past the end */
    ASSERT_EQ(GTPU_ERR_EXT, gtpu_parse(b, sizeof(b), &h));
}

TEST(build_then_parse_gpdu_roundtrip)
{
    const uint8_t inner[] = {0x45, 0x00, 0x00, 0x1C, 0xDE, 0xAD};
    uint8_t out[64];
    gtpu_hdr_t h;

    size_t n = gtpu_build_gpdu(out, sizeof(out), 0xCAFEBABEu, inner, sizeof(inner));
    ASSERT_EQ(8 + sizeof(inner), n);
    ASSERT_EQ(GTPU_OK, gtpu_parse(out, n, &h));
    ASSERT_EQ(0xCAFEBABEu, h.teid);
    ASSERT_EQ(sizeof(inner), h.payload_len);
    ASSERT_MEM_EQ(inner, out + h.hdr_len, sizeof(inner));
}

TEST(build_gpdu_rejects_small_buffer)
{
    const uint8_t inner[4] = {0};
    uint8_t out[10];
    ASSERT_EQ(0, gtpu_build_gpdu(out, sizeof(out), 1, inner, sizeof(inner)));
}

TEST(build_echo_response)
{
    uint8_t out[32];
    const uint8_t expected[] = {0x32, 0x02, 0x00, 0x06, 0x00, 0x00, 0x00,
                                0x00, 0x12, 0x34, 0x00, 0x00, 14,   0x00};
    size_t n = gtpu_build_echo_response(out, sizeof(out), 0x1234);
    ASSERT_EQ(sizeof(expected), n);
    ASSERT_MEM_EQ(expected, out, sizeof(expected));
}

int main(void)
{
    RUN(parse_plain_gpdu);
    RUN(parse_gpdu_with_qfi);
    RUN(parse_echo_request);
    RUN(reject_short_and_bad_headers);
    RUN(reject_bad_extension_chain);
    RUN(build_then_parse_gpdu_roundtrip);
    RUN(build_gpdu_rejects_small_buffer);
    RUN(build_echo_response);
    return minitest_report();
}
