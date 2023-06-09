import subprocess
import time
import argparse
import socket


def test_ping_becomes_pong_then_kill(args):
    proc = subprocess.Popen(args.executable)
    time.sleep(1)

    sock = socket.create_connection(("127.0.0.1", 8080), timeout=2)

    sock.sendall(b"PING")
    pong = sock.recv(1024).decode("utf-8")
    assert pong == "PONG", f"Expected: PONG, got: {pong}"

    sock.close()
    proc.send_signal(subprocess.signal.SIGINT)
    proc.communicate(timeout=1)


def test_all_pings_become_pong(args):
    proc = subprocess.Popen(args.executable)
    time.sleep(1)

    sock = socket.create_connection(("127.0.0.1", 8080), timeout=2)

    message = "I PING, you PONG. PING"
    expected = "I PONG, you PONG. PONG"

    sock.sendall(message.encode("utf-8"))
    pong = sock.recv(1024).decode("utf-8")
    assert pong == expected, f"Expected: ${expected}, got: {pong}"

    sock.close()
    proc.send_signal(subprocess.signal.SIGINT)
    proc.communicate(timeout=1)


def test_multiple_connections_support(args):
    proc = subprocess.Popen(args.executable)
    time.sleep(1)

    sock1 = socket.create_connection(("127.0.0.1", 8080), timeout=2)
    sock2 = socket.create_connection(("127.0.0.1", 8080), timeout=2)

    sock2.sendall(b"PI")
    sock1.sendall(b"PI")
    time.sleep(1)
    sock2.sendall(b"NG")

    pong2 = sock2.recv(1024).decode("utf-8")
    assert pong2 == "PONG"

    sock1.sendall(b"NG")
    pong1 = sock1.recv(1024).decode("utf-8")
    assert pong1 == "PONG"

    sock1.close()
    sock2.close()
    proc.send_signal(subprocess.signal.SIGINT)
    proc.communicate(timeout=1)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--executable', help='full path to executable')
    args = parser.parse_args()
    test_ping_becomes_pong_then_kill(args)
    # test_all_pings_become_pong(args)
    # test_multiple_connections_support(args)
