import subprocess
import socket
import argparse
import time
import re


def test_receive_ascii_back(args):
    proc = subprocess.Popen([args.executable])
    sock = socket.socket(socket.AF_INET)

    time.sleep(1)
    sock.connect(("127.0.0.1", 8080))

    message = "abcxyz09[(\"')]^&@?\r\n"
    sock.send(message.encode("utf-8"))
    response = sock.recv(1024)
    assert response.decode("utf-8") == message, \
           "Expected\t: %s Received\t: %s" %    \
           (re.escape(message), re.escape(response.decode("utf-8")))

    sock.close()
    proc.send_signal(subprocess.signal.SIGINT)
    proc.communicate(timeout=1)


def test_receive_utf8_back(args):
    proc = subprocess.Popen(args.executable)
    sock = socket.socket(socket.AF_INET)

    time.sleep(1)
    sock.connect(("127.0.0.1", 8080))

    message = "Take your chance to parrot me 🦜"
    sock.send(message.encode("utf-8"))
    response = sock.recv(1024)
    assert response.decode("utf-8") == message, \
           f"Expected\t: {message}\nReceived\t: {response}"

    sock.close()
    proc.send_signal(subprocess.signal.SIGINT)
    proc.communicate(timeout=1)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--executable', help='full path to executable')
    args = parser.parse_args()
    test_receive_ascii_back(args)
    test_receive_utf8_back(args)
