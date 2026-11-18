# Roadmap

Target role: user plane C developer in mobile packet core. Each milestone is
tagged in git so progress is visible from the outside.

| Week | Milestone | What it shows | Tag |
|---|---|---|---|
| 0 | Skeleton: GTP-U parser/builder, Echo, counters, unit + functional tests, CI, Docker, Jenkinsfile | C quality, test discipline | `v0.0.1` |
| 1 | Static sessions: TEID/UE-IP rule table, N6 TUN, uplink decap + downlink encap, netns end-to-end test, first pps number | Working data path | `v0.1.0` |
| 2 | PFCP subset: Heartbeat, Association Setup, Session Establishment/Modification/Deletion with Create PDR/FAR (PDI: F-TEID, UE IP; FAR: forward + Outer Header Creation). Open5GS SMF drives mini-upf; srsRAN gNB/UE over ZMQ, UE ping + iperf through mini-upf. **Apply with this.** | 3GPP + interop | `v0.2.0` |
| 3 | Performance: traffic generator in C, pps/latency/jitter measurement, perf + flame graphs, per-worker counters, libFuzzer on PFCP parser | Real-time and measurement | `v0.3.0` |
| 4 | Multi-core: N worker threads, SO_REUSEPORT or RSS-style hashing, lock-free SPSC rings, CPU pinning, scaling graph 1..N cores | Concurrency | `v0.4.0` |
| 5 | AF_XDP data path (generic + native mode on veth), comparison with the socket path | Kernel bypass | `v0.5.0` |
| 6 | Kubernetes (k3s/kind) + Helm chart, Jenkins pipeline running, QoS: QER with simple rate limit per QFI, write-up + LinkedIn post | Cloud native + summary | `v1.0.0` |

## Scope rules

- Only what is listed above; everything else goes to "Future work" in README.
- Every feature lands with a unit test and, if it touches the wire, a functional test.
- Benchmarks always state the machine, kernel, packet size and command line.
