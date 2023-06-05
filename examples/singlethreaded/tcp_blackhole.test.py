import subprocess
import time
import socket


def test():
    proc = subprocess.Popen("test/tcp_blackhole")
    time.sleep(1)

    s1 = socket.socket(socket.AF_INET)
    s1.connect(("127.0.0.1", 8080))
    s1.sendall("Hello, this message is not supposed to affect anything")

    s2 = socket.socket(socket.AF_INET)
    s2.connect(("127.0.0.1", 8080))

    s3 = socket.socket(socket.AF_INET)
    s3.connect(("127.0.0.1", 8080))

    s3.close()
    s2.close()
    s1.close()

    s4 = socket.socket(socket.AF_INET)
    s4.connect(("127.0.0.1", 8080))

    s4.close()

    proc.send_signal(subprocess.signal.SIGINT)
    proc.communicate(timeout=1)
    pass


if __name__ == "__main__":
    test()
