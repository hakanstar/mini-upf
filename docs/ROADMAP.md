# Roadmap

Milestones are tagged in git; each tag's release notes list what changed.

| Milestone | Content | Tag |
|---|---|---|
| Skeleton | GTP-U parser/builder, Echo, counters, unit + functional tests, CI, Docker, Jenkinsfile | `v0.0.1` |
| Static data path | TEID / UE-IP session table, N6 TUN, uplink decap + downlink encap, end-to-end test in network namespaces, first throughput number | `v0.1.0` |
| PFCP + interop | PFCP subset: Heartbeat, Association Setup, Session Establishment/Modification/Deletion with Create PDR/FAR (PDI: F-TEID, UE IP; FAR: forward + Outer Header Creation). Driven by the Open5GS SMF; srsRAN gNB/UE over ZMQ with ping and iperf through mini-upf | `v0.2.0` |
| Performance | C traffic generator, pps/latency/jitter measurement, perf flame graphs, per-worker counters, fuzzing of the PFCP parser | `v0.3.0` |
| Multi-core | N worker threads, SO_REUSEPORT or RSS-style hashing, lock-free SPSC rings, CPU pinning, scaling over cores | `v0.4.0` |
| AF_XDP | AF_XDP data path (generic and native mode on veth), comparison with the socket path | `v0.5.0` |
| Cloud native + QoS | Kubernetes (k3s/kind) + Helm chart, Jenkins pipeline, QER with per-QFI rate limiting, write-up of results | `v1.0.0` |

## Scope rules

- Only what is listed above; everything else goes to "Future work" in the README.
- Every feature lands with a unit test and, if it touches the wire, a functional test.
- Benchmarks always state the machine, kernel, packet size and command line.
