# Week 1 plan: from skeleton to a working data path

Goal at the end of the week (tag `v0.1.0`): a UE IP packet sent as a G-PDU on
N3 comes out of a TUN device on N6, and the reply goes back to the "gNB" as a
G-PDU with the right TEID. Proven by an automated test in network namespaces.

Each day ends with: CI green, one commit per logical change, a short line in
`docs/DEVLOG.md` (what was done, what was learned).

---

## Day 1 — Repo live, own the parser

- [ ] Create `github.com/hakanstar/mini-upf`, push the skeleton, check that every CI job is green.
- [ ] Read TS 29.281 §5 (header, extension headers) and §7.2 (Echo); map each field to `gtpu_parse()`.
- [ ] Add unit tests: S+PN flags together, two chained extension headers, trailing bytes after Length, Length smaller than the 4 optional octets.
- [ ] Run `make fuzz` locally for 10 minutes; add any crash input as a regression test.
- [ ] Capture the functional tests with `tcpdump -i lo -w docs/captures/day1.pcap udp`, open in Wireshark, check that it decodes Echo Request/Response correctly.

**Done when:** ≥12 unit tests, fuzz run clean, pcap committed.

## Day 2 — N6 side: TUN device

- [ ] `src/tun.c` / `include/upf/tun.h`: `tun_open(name)`, `tun_read()`, `tun_write()` (IFF_TUN | IFF_NO_PI).
- [ ] CLI option `-t upf0`; set address and route with `ip` in a helper script `scripts/n6-up.sh`.
- [ ] Uplink path in `main.c`: G-PDU → write `buf + hdr_len` to TUN, count `tx_n6_pkts`.
- [ ] Move the receive loop to `poll()` over two fds (N3 socket, TUN).
- [ ] Manual test: send a G-PDU with an inner ICMP echo, see it with `tcpdump -i upf0`.

**Done when:** inner packets appear on `upf0`; `poll()` loop handles both fds.

## Day 3 — Session table (static rules)

- [ ] `src/rules.c` / `include/upf/rules.h`: session = {UE IPv4, uplink TEID, gNB IPv4, downlink TEID, QFI}.
- [ ] Two lookups: by uplink TEID (uplink) and by UE IP (downlink); open-addressing hash table, fixed capacity, no malloc in the fast path.
- [ ] Load sessions from `config/sessions.conf` (one line per session); `-c` option.
- [ ] Unknown TEID: drop and count `rx_unknown_teid`; optional: send Error Indication (TS 29.281 §7.3.1).
- [ ] Unit tests: insert/lookup/delete, collisions, full table, unknown keys.

**Done when:** uplink only forwards packets whose TEID matches a session and whose inner source IP matches the UE IP.

## Day 4 — Downlink and end-to-end test

- [ ] Downlink: read IP packet from TUN → look up destination IP → `gtpu_build_gpdu()` with the session's downlink TEID → `sendto()` gNB, count `tx_n3_pkts`. Build in place (headroom) to avoid a copy.
- [ ] `tests/e2e/run.sh` (root): namespaces `gnb`, `upf`, `dn`, veth pairs; Scapy "gNB" sends an encapsulated ping to a host in `dn`, expects an encapsulated reply with the right TEID.
- [ ] pytest wrapper `tests/e2e/test_ping.py`, marked `e2e`; new CI job with `sudo`.

**Done when:** full round trip passes in CI.

## Day 5 — Robustness and first number

- [ ] `SIGUSR1` prints counters without stopping; structured log lines (`level=… event=…`).
- [ ] Malformed inner packets (short IP header, wrong version) on both paths: drop + count, test.
- [ ] `tools/gtpu-gen.c`: sends N G-PDUs of size S as fast as possible; measure pps on loopback for 64/512/1400-byte payloads.
- [ ] Write `docs/BENCHMARKS.md` v0 with machine, kernel, command and result (honest baseline, no tuning yet).

**Done when:** a first pps table exists and every drop reason has a counter and a test.

## Day 6 — Packaging and documentation

- [ ] Dockerfile runs with `--cap-add NET_ADMIN --device /dev/net/tun`; `docker-compose.yml` for gnb-sim + upf.
- [ ] README: updated architecture diagram, how to reproduce the e2e test, link to pcaps and benchmark.
- [ ] Code coverage: `gcov`/`lcov` job in CI, coverage summary in README.
- [ ] Tag `v0.1.0` with release notes.

## Day 7 — Review and buffer

- [ ] Self code review: error paths, integer conversions, buffer bounds, `valgrind` run.
- [ ] Catch up on anything left from Days 1–6.
- [ ] Read TS 29.244 §7.2–7.5 (PFCP header, Association Setup, Session Establishment) for week 2; list the IEs needed.
- [ ] LinkedIn draft (do not post yet): what was built, one diagram, one measured number.

---

### Week 2 preview

PFCP over UDP/8805: Heartbeat, Association Setup, Session Establishment with
Create PDR / Create FAR; replace `sessions.conf` with sessions the Open5GS SMF
creates; srsRAN gNB + UE over ZMQ; UE `ping` and `iperf3` through mini-upf.
End of week 2: tag `v0.2.0` and apply.
