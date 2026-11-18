/* SPDX-License-Identifier: MIT */
#include "upf/gtpu.h"

#include <string.h>

static uint16_t rd16(const uint8_t *p)
{
    return (uint16_t)((p[0] << 8) | p[1]);
}

static uint32_t rd32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) |
           (uint32_t)p[3];
}

static void wr16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)(v >> 8);
    p[1] = (uint8_t)v;
}

static void wr32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
}

/*
 * Walk the extension header chain starting at 'p' (first ext header),
 * 'end' is one past the last byte covered by the Length field.
 * Each ext header: [len in 4-octet units][content ...][next type].
 */
static int parse_ext_chain(const uint8_t *p, const uint8_t *end, uint8_t next,
                           gtpu_hdr_t *out, const uint8_t **after)
{
    unsigned guard = 0;

    while (next != GTPU_EXT_NONE) {
        if (++guard > 16) /* no sane packet has more */
            return GTPU_ERR_EXT;
        if (p >= end)
            return GTPU_ERR_EXT;

        size_t ext_len = (size_t)p[0] * 4u;
        if (ext_len == 0 || ext_len > (size_t)(end - p))
            return GTPU_ERR_EXT;

        if (next == GTPU_EXT_PDU_SESSION_CONTAINER && ext_len >= 4) {
            /* octet 1: PDU type | spare; octet 2: (PPP|RQI|)QFI(6 bits) */
            out->has_qfi = true;
            out->qfi = p[2] & 0x3Fu;
        }

        next = p[ext_len - 1];
        p += ext_len;
    }

    *after = p;
    return GTPU_OK;
}

int gtpu_parse(const uint8_t *buf, size_t len, gtpu_hdr_t *out)
{
    if (!buf || !out || len < GTPU_HDR_LEN)
        return GTPU_ERR_SHORT;

    memset(out, 0, sizeof(*out));
    out->flags = buf[0];
    out->type = buf[1];
    out->length = rd16(buf + 2);
    out->teid = rd32(buf + 4);

    if ((out->flags >> 5) != 1)
        return GTPU_ERR_VERSION;
    if (!(out->flags & GTPU_FLAG_PT))
        return GTPU_ERR_PT;
    if ((size_t)out->length > len - GTPU_HDR_LEN)
        return GTPU_ERR_LENGTH;

    const uint8_t *p = buf + GTPU_HDR_LEN;
    const uint8_t *end = p + out->length; /* trailing bytes beyond Length are ignored */

    out->has_opt = (out->flags & (GTPU_FLAG_E | GTPU_FLAG_S | GTPU_FLAG_PN)) != 0;
    if (out->has_opt) {
        if (out->length < GTPU_OPT_LEN)
            return GTPU_ERR_LENGTH;
        out->seq = rd16(p);
        out->npdu = p[2];
        uint8_t next = (out->flags & GTPU_FLAG_E) ? p[3] : GTPU_EXT_NONE;
        p += GTPU_OPT_LEN;

        int rc = parse_ext_chain(p, end, next, out, &p);
        if (rc != GTPU_OK)
            return rc;
    }

    out->hdr_len = (size_t)(p - buf);
    out->payload_len = (size_t)(end - p);
    return GTPU_OK;
}

size_t gtpu_build_gpdu(uint8_t *out, size_t cap, uint32_t teid, const uint8_t *payload,
                       size_t plen)
{
    if (!out || plen > 0xFFFFu || cap < GTPU_HDR_LEN + plen)
        return 0;
    if (plen && !payload)
        return 0;

    out[0] = GTPU_FLAG_VERSION_1 | GTPU_FLAG_PT;
    out[1] = GTPU_G_PDU;
    wr16(out + 2, (uint16_t)plen);
    wr32(out + 4, teid);
    if (plen)
        memmove(out + GTPU_HDR_LEN, payload, plen); /* payload may alias out */
    return GTPU_HDR_LEN + plen;
}

size_t gtpu_build_echo_response(uint8_t *out, size_t cap, uint16_t seq)
{
    const size_t total = GTPU_HDR_LEN + GTPU_OPT_LEN + 2; /* + Recovery IE */
    if (!out || cap < total)
        return 0;

    out[0] = GTPU_FLAG_VERSION_1 | GTPU_FLAG_PT | GTPU_FLAG_S;
    out[1] = GTPU_ECHO_RESPONSE;
    wr16(out + 2, (uint16_t)(total - GTPU_HDR_LEN));
    wr32(out + 4, 0);
    wr16(out + 8, seq);
    out[10] = 0;             /* N-PDU number */
    out[11] = GTPU_EXT_NONE; /* next extension header type */
    out[12] = GTPU_IE_RECOVERY;
    out[13] = 0; /* restart counter: shall be 0 in GTPv1-U */
    return total;
}

const char *gtpu_strerror(int err)
{
    switch (err) {
    case GTPU_OK: return "ok";
    case GTPU_ERR_SHORT: return "datagram too short";
    case GTPU_ERR_VERSION: return "not GTP version 1";
    case GTPU_ERR_PT: return "protocol type is not GTP";
    case GTPU_ERR_LENGTH: return "length field inconsistent";
    case GTPU_ERR_EXT: return "malformed extension header";
    default: return "unknown error";
    }
}
