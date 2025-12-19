#!/usr/bin/env python3
import argparse
import ipaddress
import selectors
import socket
import struct
import threading
import types

PPV2_MAGIC = b"\r\n\r\n\x00\r\nQUIT\n"
PPV2_VER_CMD_PROXY = 0x20 | 0x01  # version 2, PROXY command
PPV2_AF_INET_STREAM = 0x10 | 0x01  # AF_INET + STREAM
PPV2_AF_INET6_STREAM = 0x20 | 0x01  # AF_INET6 + STREAM


def make_ppv2_header(src_ip: str, src_port: int, dst_ip: str, dst_port: int) -> bytes:
    src = ipaddress.ip_address(src_ip)
    dst = ipaddress.ip_address(dst_ip)
    if src.version != dst.version:
        raise ValueError("src and dst IP versions must match for PPv2")
    ver_cmd = bytes([PPV2_VER_CMD_PROXY])
    if src.version == 4:
        fam_proto = bytes([PPV2_AF_INET_STREAM])
        addr = socket.inet_pton(socket.AF_INET, src_ip) + socket.inet_pton(socket.AF_INET, dst_ip)
        ports = struct.pack("!HH", src_port, dst_port)
        tlv_len = struct.pack("!H", 12)
    else:
        fam_proto = bytes([PPV2_AF_INET6_STREAM])
        addr = socket.inet_pton(socket.AF_INET6, src_ip) + socket.inet_pton(socket.AF_INET6, dst_ip)
        ports = struct.pack("!HH", src_port, dst_port)
        tlv_len = struct.pack("!H", 36)
    return PPV2_MAGIC + ver_cmd + fam_proto + tlv_len + addr + ports


def make_ppv1_header(src_ip: str, src_port: int, dst_ip: str, dst_port: int) -> bytes:
    src = ipaddress.ip_address(src_ip)
    fam = "TCP4" if src.version == 4 else "TCP6"
    line = f"PROXY {fam} {src_ip} {dst_ip} {src_port} {dst_port}\r\n"
    return line.encode("ascii")


def pump_bidirectional(a: socket.socket, b: socket.socket):
    sel = selectors.DefaultSelector()
    a.setblocking(False)
    b.setblocking(False)
    sel.register(a, selectors.EVENT_READ, data=types.SimpleNamespace(peer=b))
    sel.register(b, selectors.EVENT_READ, data=types.SimpleNamespace(peer=a))
    try:
        while True:
            events = sel.select(timeout=60)
            if not events:
                # Idle timeout; keep alive by continuing
                continue
            for key, _ in events:
                src_sock = key.fileobj
                peer_sock = key.data.peer
                try:
                    data = src_sock.recv(4096)
                except (BlockingIOError, InterruptedError):
                    continue
                if not data:
                    return
                try:
                    peer_sock.sendall(data)
                except BrokenPipeError:
                    return
    finally:
        try:
            sel.unregister(a)
        except Exception:
            pass
        try:
            sel.unregister(b)
        except Exception:
            pass
        a.close()
        b.close()


def handle_client(client_sock: socket.socket, upstream_host: str, upstream_port: int, version: str, src_ip: str, src_port: int):
    try:
        upstream = socket.create_connection((upstream_host, upstream_port))
        # Build and send PP header
        dst_ip = upstream.getsockname()[0]
        dst_port = upstream.getsockname()[1]
        if version == "v2":
            header = make_ppv2_header(src_ip, src_port, dst_ip, dst_port)
        else:
            header = make_ppv1_header(src_ip, src_port, dst_ip, dst_port)
        upstream.sendall(header)
        # Now pump data both ways
        pump_bidirectional(client_sock, upstream)
    except Exception:
        try:
            client_sock.close()
        except Exception:
            pass


def serve(listen_host: str, listen_port: int, upstream_host: str, upstream_port: int, version: str, src_ip: str, src_port: int):
    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind((listen_host, listen_port))
    srv.listen(128)
    print(f"PP proxy {version} listening on {listen_host}:{listen_port} -> {upstream_host}:{upstream_port} as {src_ip}:{src_port}")
    try:
        while True:
            client, addr = srv.accept()
            t = threading.Thread(target=handle_client, args=(client, upstream_host, upstream_port, version, src_ip, src_port), daemon=True)
            t.start()
    finally:
        srv.close()


def main():
    parser = argparse.ArgumentParser(description="Proxy Protocol v1/v2 injecting TCP proxy")
    parser.add_argument("--version", choices=["v1", "v2"], required=True)
    parser.add_argument("--listen-host", default="0.0.0.0")
    parser.add_argument("--listen-port", type=int, required=True)
    parser.add_argument("--upstream-host", required=True)
    parser.add_argument("--upstream-port", type=int, required=True)
    parser.add_argument("--src-ip", required=True)
    parser.add_argument("--src-port", type=int, required=True)
    args = parser.parse_args()

    serve(args.listen_host, args.listen_port, args.upstream_host, args.upstream_port, args.version, args.src_ip, args.src_port)


if __name__ == "__main__":
    main()
