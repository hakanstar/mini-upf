# SPDX-License-Identifier: MIT
"""N3 interface basics: echo, G-PDU accounting, malformed input."""
import time

from scapy.contrib.gtp import GTP_U_Header, GTPEchoRequest, GTPEchoResponse
from scapy.layers.inet import ICMP, IP


def _send(gnb, upf, pkt):
    gnb.sendto(bytes(pkt), upf.addr)


def test_echo_request_gets_response(upf, gnb):
    req = GTP_U_Header(S=1, seq=0x1234, teid=0) / GTPEchoRequest()
    _send(gnb, upf, req)

    data, _ = gnb.recvfrom(2048)
    resp = GTP_U_Header(data)
    assert resp.gtp_type == 2  # Echo Response
    assert resp.seq == 0x1234
    assert resp.teid == 0
    assert resp.haslayer(GTPEchoResponse)

    stats = upf.stop()
    assert stats["rx_echo_req"] == 1
    assert stats["tx_echo_resp"] == 1


def test_gpdu_is_counted(upf, gnb):
    inner = IP(src="10.45.0.2", dst="8.8.8.8") / ICMP()
    for i in range(5):
        _send(gnb, upf, GTP_U_Header(teid=0x100 + i) / inner)

    time.sleep(0.2)
    stats = upf.stop()
    assert stats["rx_gpdu"] == 5
    assert stats["rx_malformed"] == 0


def test_malformed_datagrams_are_dropped(upf, gnb):
    gnb.sendto(b"\x30\xff", upf.addr)                     # too short
    gnb.sendto(b"\x50\xff\x00\x00\x00\x00\x00\x01", upf.addr)  # version 2
    gnb.sendto(b"\x30\xff\x00\x40\x00\x00\x00\x01", upf.addr)  # length too big

    # UPF must still answer afterwards
    _send(gnb, upf, GTP_U_Header(S=1, seq=7, teid=0) / GTPEchoRequest())
    data, _ = gnb.recvfrom(2048)
    assert GTP_U_Header(data).gtp_type == 2

    stats = upf.stop()
    assert stats["rx_malformed"] == 3
