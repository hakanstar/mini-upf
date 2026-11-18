# mini-upf

[![CI](https://github.com/hakanstar/mini-upf/actions/workflows/ci.yml/badge.svg)](https://github.com/hakanstar/mini-upf/actions/workflows/ci.yml)

A small **5G User Plane Function (UPF) written in C**: GTP-U on N3, IP on N6,
PFCP on N4. Built to learn and demonstrate mobile packet-switching data paths:
header processing, session rule lookup, multi-core fast paths and measurement.

> Learning/portfolio project. It implements a deliberately small subset of
> 3GPP TS 29.281 (GTP-U) and TS 29.244 (PFCP); it is not a compliant or
> production UPF.

## Architecture

```
            N3 (GTP-U / UDP 2152)                N6 (plain IP)
  gNB  <------------------------->  mini-upf  <---------------->  Data network
 (srsRAN / Scapy)                   |   ^                          (TUN device)
                                    |   |  N4 (PFCP / UDP 8805)
                                    v   |
                                  SMF (Open5GS)

  uplink:   G-PDU --parse--> PDR lookup (TEID) --FAR--> decap --> TUN
  downlink: TUN --> PDR lookup (UE IP) --FAR--> encap (gNB TEID) --> N3
```

| Module | File | Status |
|---|---|---|
| GTP-U parse/build, ext headers, QFI | `src/gtpu.c` | done |
| Echo Request/Response (path management) | `src/main.c` | done |
| Counters (JSON) | `src/stats.c` | done |
| N6 TUN interface | `src/tun.c` | planned |
| Session/rule table (TEID, UE IP) | `src/rules.c` | planned |
| PFCP association + session establishment | `src/pfcp.c` | planned |
| Multi-worker fast path, lock-free rings | `src/worker.c` | planned |
| AF_XDP data path | `src/xsk.c` | planned |

See [docs/ROADMAP.md](docs/ROADMAP.md).

## Build and test

Requirements: Linux (or WSL2), gcc or clang, make, Python 3.10+.

```sh
pip install -r requirements-dev.txt
make              # build/mini-upf
make test         # C unit tests
make functest     # pytest + Scapy functional tests (no root needed)
make SANITIZE=1 test functest   # with AddressSanitizer + UBSan
make lint         # cppcheck + clang-format
make fuzz && ./build/fuzz_gtpu -max_total_time=60   # libFuzzer
```

## Run

```sh
./build/mini-upf -a 127.0.0.1 -p 2152 -v
# Ctrl-C prints counters as one JSON line:
# {"rx_pkts":3,"rx_gpdu":2,"rx_echo_req":1,"tx_echo_resp":1,...}
```

Send a GTP-U Echo Request from Python:

```python
from scapy.contrib.gtp import GTP_U_Header, GTPEchoRequest
import socket
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.sendto(bytes(GTP_U_Header(S=1, seq=1) / GTPEchoRequest()), ("127.0.0.1", 2152))
print(s.recv(64).hex())
```

Container:

```sh
docker build -t mini-upf .
docker run --rm -p 2152:2152/udp mini-upf -v
```

## Testing strategy

- **Unit tests** (`tests/unit`): parser and builders against hand-made byte vectors.
- **Functional tests** (`tests/func`): the real binary on a free localhost port,
  driven with Scapy-built packets; counters are checked from the JSON output.
- **Sanitizers, static analysis, fuzzing** run in CI on every push.
- **End-to-end** (planned): Open5GS SMF + srsRAN gNB/UE over ZMQ, traffic from the
  UE through mini-upf; pcaps under `docs/captures/`.

## Results

Benchmarks (packets per second, latency percentiles, scaling over cores) will be
published in `docs/BENCHMARKS.md`.

## References

- 3GPP TS 29.281 — GPRS Tunnelling Protocol User Plane (GTPv1-U)
- 3GPP TS 29.244 — Interface between the Control Plane and the User Plane nodes (PFCP)
- 3GPP TS 23.501 — System architecture for the 5G System
- 3GPP TS 38.415 — PDU Session User Plane protocol (QFI in the PDU Session Container)

## License

MIT
