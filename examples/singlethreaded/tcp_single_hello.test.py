import subprocess
import argparse
import socket
import time


def test_to_recv_hello(args):
    proc = subprocess.Popen([args.executable])
    sock = socket.socket(socket.AF_INET)
    time.sleep(1)
    sock.connect(("127.0.0.1", 8080))
    response = sock.recv(1024)
    assert response == b"hello", f"Received not a \"hello\" but {response}"
    sock.close()
    proc.communicate()  # immedately closed
    pass


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--executable', help='full path to executable')
    args = parser.parse_args()
    test_to_recv_hello(args)
