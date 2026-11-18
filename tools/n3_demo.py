#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Send a short, readable N3 exchange to a running mini-upf on UDP/2152.

Used to produce docs/captures/n3_demo.pcap, which Wireshark decodes as GTP-U
because both ends use the standard port:

    ./build/mini-upf -a 127.0.0.1 &
    sudo tcpdump -i lo -w docs/captures/n3_demo.pcap udp port 2152 &
    python3 tools/n3_demo.py
"""
import argparse
import socket

from scapy.contrib.gtp import GTP_U_Header, GTPEchoRequest, GTPPDUSessionContainer
from scapy.layers.inet import ICMP, IP


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--upf", default="127.0.0.1")
    ap.add_argument("--port", type=int, default=2152)
    ap.add_argument("--src", default="127.0.0.2", help="gNB-side address to bind")
    args = ap.parse_args()

    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.bind((args.src, args.port))
    s.settimeout(1.0)
    upf = (args.upf, args.port)

    # 1. Path management: Echo Request -> Echo Response
    s.sendto(bytes(GTP_U_Header(S=1, seq=1, teid=0) / GTPEchoRequest()), upf)
    print("echo response:", s.recv(64).hex())

    # 2. Uplink user data: plain G-PDU carrying an ICMP echo from the UE
    inner = IP(src="10.45.0.2", dst="10.60.0.1") / ICMP(id=1, seq=1)
    s.sendto(bytes(GTP_U_Header(teid=0x00000101) / inner), upf)

    # 3. Uplink G-PDU with a PDU Session Container (QFI 9)
    pkt = (
        GTP_U_Header(E=1, next_ex=0x85, teid=0x00000101)
        / GTPPDUSessionContainer(type=1, QFI=9)
        / IP(src="10.45.0.2", dst="10.60.0.1")
        / ICMP(id=1, seq=2)
    )
    s.sendto(bytes(pkt), upf)

    # 4. Malformed datagram (version 2): must be dropped and counted
    s.sendto(b"\x50\xff\x00\x00\x00\x00\x00\x01", upf)
    print("sent 4 datagrams")


if __name__ == "__main__":
    main()
