/* SPDX-License-Identifier: MIT */
/*
 * GTP-U (GTPv1-U) header handling, 3GPP TS 29.281.
 *
 * Wire format of the mandatory header (8 octets):
 *
 *   0                   1                   2                   3
 *   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *  +-----+-+-+-+-+-+---------------+-------------------------------+
 *  | Ver |P|*|E|S|N| Message Type  |            Length             |
 *  +-----+-+-+-+-+-+---------------+-------------------------------+
 *  |              Tunnel Endpoint Identifier (TEID)                |
 *  +---------------------------------------------------------------+
 *
 * If any of E, S or PN is set, 4 more octets follow:
 *  | Sequence Number (16) | N-PDU Number (8) | Next Ext Hdr Type (8) |
 *
 * "Length" counts every octet after the first 8, i.e. optional fields,
 * extension headers and the T-PDU.
 */
#ifndef UPF_GTPU_H
#define UPF_GTPU_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define GTPU_PORT    2152u
#define GTPU_HDR_LEN 8u
#define GTPU_OPT_LEN 4u

/* Flags (first octet) */
#define GTPU_FLAG_VERSION_1 0x20u /* version = 1 in the top 3 bits */
#define GTPU_FLAG_PT        0x10u /* protocol type: 1 = GTP */
#define GTPU_FLAG_E         0x04u /* extension header present */
#define GTPU_FLAG_S         0x02u /* sequence number present */
#define GTPU_FLAG_PN        0x01u /* N-PDU number present */

/* Message types used by the user plane */
enum gtpu_msg_type {
    GTPU_ECHO_REQUEST = 1,
    GTPU_ECHO_RESPONSE = 2,
    GTPU_ERROR_INDICATION = 26,
    GTPU_SUPPORTED_EXT_HDR_NOTIF = 31,
    GTPU_END_MARKER = 254,
    GTPU_G_PDU = 255,
};

/* Extension header types (TS 29.281 §5.2.1) */
#define GTPU_EXT_NONE                  0x00u
#define GTPU_EXT_PDU_SESSION_CONTAINER 0x85u /* carries QFI, TS 38.415 */

/* Information element used in Echo Response */
#define GTPU_IE_RECOVERY 14u

/* Parse result codes (negative = error) */
enum gtpu_err {
    GTPU_OK = 0,
    GTPU_ERR_SHORT = -1,   /* buffer shorter than the header says */
    GTPU_ERR_VERSION = -2, /* not GTPv1 */
    GTPU_ERR_PT = -3,      /* PT=0 (GTP') */
    GTPU_ERR_LENGTH = -4,  /* length field inconsistent with datagram */
    GTPU_ERR_EXT = -5,     /* malformed extension header chain */
};

/* Decoded header. All multi-octet fields are in host byte order. */
typedef struct gtpu_hdr {
    uint8_t flags;
    uint8_t type;
    uint16_t length; /* value of the Length field */
    uint32_t teid;
    bool has_opt;       /* E, S or PN was set: 4 optional octets present */
    uint16_t seq;       /* valid if (flags & GTPU_FLAG_S) */
    uint8_t npdu;       /* valid if (flags & GTPU_FLAG_PN) */
    bool has_qfi;       /* PDU Session Container found */
    uint8_t qfi;        /* QoS Flow Identifier (6 bits) */
    size_t hdr_len;     /* bytes from start of datagram to the T-PDU */
    size_t payload_len; /* bytes of T-PDU (inner packet) */
} gtpu_hdr_t;

/*
 * Parse a GTP-U datagram (the UDP payload).
 * On success returns GTPU_OK and fills *out; the inner packet starts at
 * buf + out->hdr_len and is out->payload_len bytes long.
 */
int gtpu_parse(const uint8_t *buf, size_t len, gtpu_hdr_t *out);

/*
 * Build a G-PDU (uplink/downlink user data) without optional fields.
 * Returns the number of bytes written, or 0 if 'cap' is too small.
 */
size_t gtpu_build_gpdu(uint8_t *out, size_t cap, uint32_t teid, const uint8_t *payload,
                       size_t plen);

/*
 * Build an Echo Response for a request that carried sequence number 'seq'.
 * TEID is 0 and a Recovery IE (restart counter 0) is appended, as
 * TS 29.281 §7.2.2 requires. Returns bytes written or 0.
 */
size_t gtpu_build_echo_response(uint8_t *out, size_t cap, uint16_t seq);

/* Human-readable name for an error code (for logs). */
const char *gtpu_strerror(int err);

#endif /* UPF_GTPU_H */
