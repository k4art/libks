import subprocess
import time
import socket


def test():
    proc = subprocess.Popen("test/tcp_ping_pong_server")
    time.sleep(1)

    sock = socket.socket(socket.AF_INET)
    sock.connect(("127.0.0.1", 8080))

    sock.sendall("PING")
    pong = sock.recv(1024)
    assert pong == b"PONG", f"Expected: PONG, got: {pong}"

    sock.sendall("This is PING")
    pong = sock.recv(1024)
    assert pong == b"This is PONG", f"Expected: This is PONG, got: {pong}"

    sock.sendall("[PI")
    time.sleep(1)
    sock.sendall("NG]")
    pong = sock.recv(1024)
    assert pong == b"[PONG]", f"Expected: [PONG], got: {pong}"

    sock2 = socket.socket(socket.AF_INET)
    sock2.connect(("127.0.0.1", 8080))
    sock2.sendall("PING")
    pong = sock2.recv(1024)
    assert pong == b"PONG", f"Expected: PONG, got: {pong}"

    sock2.close()
    sock.close()

    proc.send_signal(subprocess.signal.SIGINT)
    proc.communicate(timeout=1)


if __name__ == "__main__":
    test()
