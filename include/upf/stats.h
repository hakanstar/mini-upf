/* SPDX-License-Identifier: MIT */
#ifndef UPF_STATS_H
#define UPF_STATS_H

#include <stdint.h>
#include <stdio.h>

/* Per-process counters. Week 3: make these per-worker and lock-free. */
typedef struct upf_stats {
    uint64_t rx_pkts;
    uint64_t rx_bytes;
    uint64_t rx_gpdu;
    uint64_t rx_echo_req;
    uint64_t tx_echo_resp;
    uint64_t rx_end_marker;
    uint64_t rx_unknown_type;
    uint64_t rx_malformed;
    uint64_t rx_unknown_teid; /* Day 3: no matching rule */
    uint64_t tx_n6_pkts;      /* Day 4: decapsulated and sent to N6 (TUN) */
    uint64_t tx_n3_pkts;      /* Day 4: encapsulated and sent to gNB */
} upf_stats_t;

/* One JSON object on one line, easy to parse from tests and scripts. */
void upf_stats_print_json(const upf_stats_t *s, FILE *f);

#endif /* UPF_STATS_H */
