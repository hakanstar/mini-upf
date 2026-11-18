/* SPDX-License-Identifier: MIT */
/*
 * mini-upf: a small 5G user plane function in C.
 *
 * Current state (skeleton): listens on the N3 interface (UDP/2152),
 * parses GTP-U, answers Echo Requests and counts G-PDUs.
 * Week 1 adds: static TEID rule table (Day 3) and N3 <-> N6 forwarding
 * through a TUN device (Day 4). See docs/WEEK1.md.
 */
#include "upf/gtpu.h"
#include "upf/stats.h"

#include <arpa/inet.h>
#include <errno.h>
#include <getopt.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define RX_BUF_SIZE 65536

static volatile sig_atomic_t g_stop;

static void on_signal(int sig)
{
    (void)sig;
    g_stop = 1;
}

struct config {
    const char *n3_addr;
    uint16_t n3_port;
    int verbose;
};

static void usage(const char *prog)
{
    fprintf(stderr,
            "usage: %s [-a N3_ADDR] [-p N3_PORT] [-v]\n"
            "  -a  N3 bind address (default 0.0.0.0)\n"
            "  -p  N3 UDP port (default %u)\n"
            "  -v  verbose: log every packet to stderr\n"
            "Counters are printed as one JSON line on stdout at exit (SIGINT/SIGTERM).\n",
            prog, GTPU_PORT);
}

static int parse_args(int argc, char **argv, struct config *cfg)
{
    int opt;
    while ((opt = getopt(argc, argv, "a:p:vh")) != -1) {
        switch (opt) {
        case 'a': cfg->n3_addr = optarg; break;
        case 'p': {
            char *end = NULL;
            long v = strtol(optarg, &end, 10);
            if (!end || *end != '\0' || v <= 0 || v > 65535) {
                fprintf(stderr, "invalid port: %s\n", optarg);
                return -1;
            }
            cfg->n3_port = (uint16_t)v;
            break;
        }
        case 'v': cfg->verbose = 1; break;
        default: usage(argv[0]); return -1;
        }
    }
    return 0;
}

static int open_n3_socket(const struct config *cfg)
{
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        perror("socket");
        return -1;
    }

    int one = 1;
    (void)setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(cfg->n3_port);
    if (inet_pton(AF_INET, cfg->n3_addr, &sa.sin_addr) != 1) {
        fprintf(stderr, "invalid N3 address: %s\n", cfg->n3_addr);
        close(fd);
        return -1;
    }
    if (bind(fd, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
        perror("bind");
        close(fd);
        return -1;
    }
    return fd;
}

static void handle_datagram(int fd, const uint8_t *buf, size_t len,
                            const struct sockaddr_in *peer, const struct config *cfg,
                            upf_stats_t *st)
{
    gtpu_hdr_t h;
    int rc = gtpu_parse(buf, len, &h);

    st->rx_pkts++;
    st->rx_bytes += len;

    if (rc != GTPU_OK) {
        st->rx_malformed++;
        if (cfg->verbose)
            fprintf(stderr, "drop: %s (%zu bytes)\n", gtpu_strerror(rc), len);
        return;
    }

    switch (h.type) {
    case GTPU_ECHO_REQUEST: {
        uint8_t resp[32];
        st->rx_echo_req++;
        size_t n = gtpu_build_echo_response(resp, sizeof(resp), h.seq);
        if (n && sendto(fd, resp, n, 0, (const struct sockaddr *)peer, sizeof(*peer)) ==
                     (ssize_t)n)
            st->tx_echo_resp++;
        break;
    }
    case GTPU_G_PDU:
        st->rx_gpdu++;
        if (cfg->verbose)
            fprintf(stderr, "G-PDU teid=0x%08x qfi=%d inner=%zu bytes\n", h.teid,
                    h.has_qfi ? h.qfi : -1, h.payload_len);
        /* TODO Day 3: look up h.teid in the rule table (PDR -> FAR).
         * TODO Day 4: write buf + h.hdr_len (inner IP packet) to the N6 TUN. */
        break;
    case GTPU_END_MARKER: st->rx_end_marker++; break;
    default: st->rx_unknown_type++; break;
    }
}

int main(int argc, char **argv)
{
    struct config cfg = {.n3_addr = "0.0.0.0", .n3_port = GTPU_PORT, .verbose = 0};
    if (parse_args(argc, argv, &cfg) < 0)
        return 2;

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_signal; /* no SA_RESTART: recvfrom returns EINTR */
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    int fd = open_n3_socket(&cfg);
    if (fd < 0)
        return 1;

    fprintf(stderr, "mini-upf: N3 listening on %s:%u\n", cfg.n3_addr, cfg.n3_port);

    static uint8_t buf[RX_BUF_SIZE];
    upf_stats_t st;
    memset(&st, 0, sizeof(st));

    while (!g_stop) {
        struct sockaddr_in peer;
        socklen_t plen = sizeof(peer);
        ssize_t n = recvfrom(fd, buf, sizeof(buf), 0, (struct sockaddr *)&peer, &plen);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            perror("recvfrom");
            break;
        }
        handle_datagram(fd, buf, (size_t)n, &peer, &cfg, &st);
    }

    close(fd);
    upf_stats_print_json(&st, stdout);
    return 0;
}
