# Dev log

- 2026-11-18: skeleton (GTP-U parser/builder, Echo, counters, unit + functional tests, CI, Docker, Jenkinsfile)
- 2026-11-18: GTP-U parser coverage extended (S+PN flags, chained extension headers, trailing bytes, short Length, endless extension chain, NULL arguments): 14 unit tests. libFuzzer run of 9 min, ~417M inputs, no crashes; seed corpus committed. Wireshark-decodable N3 capture added (tools/n3_demo.py, docs/captures/n3_demo.pcap).
