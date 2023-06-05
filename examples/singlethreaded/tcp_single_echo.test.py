import subprocess
import socket
import time


def test():
    sys = subprocess.Popen("test/tcp_single_echo")
    sock = socket.socket(socket.AF_INET)

    sock.connect(("127.0.0.1", 8080))
    time.sleep(1)

    message = b"Take your chance to parrot me :-)"
    sock.send(message)
    response = sock.recv(1024)
    assert response == message, \
           f"Expected\t: {message}\nReceived\t: {response}"

    sys.communicate(timeout=1)


if __name__ == "__main__":
    test()
