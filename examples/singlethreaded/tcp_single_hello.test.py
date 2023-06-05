import subprocess
import socket
import time


def test():
    proc = subprocess.Popen("test/tcp_single_hello", 8080)
    time.sleep(1)
    s = socket.socket(socket.AF_INET)
    s.connect(("127.0.0.1", 8080))
    response = s.recv(1024)
    assert response == b"hello", f"Received not a \"hello\" but {response}"
    proc.communicate()  # immedately closed
    s.close()
    pass


if __name__ == "__main__":
    test()
