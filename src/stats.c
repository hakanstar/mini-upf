/* SPDX-License-Identifier: MIT */
#include "upf/stats.h"

#include <inttypes.h>

void upf_stats_print_json(const upf_stats_t *s, FILE *f)
{
    fprintf(f,
            "{\"rx_pkts\":%" PRIu64 ",\"rx_bytes\":%" PRIu64 ",\"rx_gpdu\":%" PRIu64
            ",\"rx_echo_req\":%" PRIu64 ",\"tx_echo_resp\":%" PRIu64
            ",\"rx_end_marker\":%" PRIu64 ",\"rx_unknown_type\":%" PRIu64
            ",\"rx_malformed\":%" PRIu64 ",\"rx_unknown_teid\":%" PRIu64
            ",\"tx_n6_pkts\":%" PRIu64 ",\"tx_n3_pkts\":%" PRIu64 "}\n",
            s->rx_pkts, s->rx_bytes, s->rx_gpdu, s->rx_echo_req, s->tx_echo_resp,
            s->rx_end_marker, s->rx_unknown_type, s->rx_malformed, s->rx_unknown_teid,
            s->tx_n6_pkts, s->tx_n3_pkts);
    fflush(f);
}
