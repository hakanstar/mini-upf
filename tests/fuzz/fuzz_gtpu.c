/* SPDX-License-Identifier: MIT */
/* libFuzzer target for the GTP-U parser. Build: make fuzz; run: ./build/fuzz_gtpu */
#include "upf/gtpu.h"

#include <stddef.h>
#include <stdint.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    gtpu_hdr_t h;
    if (gtpu_parse(data, size, &h) == GTPU_OK) {
        /* invariants that must hold for every accepted packet */
        if (h.hdr_len + h.payload_len > size)
            __builtin_trap();
        if (h.hdr_len < GTPU_HDR_LEN)
            __builtin_trap();
    }
    return 0;
}
