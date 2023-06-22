import subprocess
import argparse
import time
import http.client


def test_to_recv_hello(args):
    proc = subprocess.Popen([args.executable])
    time.sleep(1)

    conn = http.client.HTTPConnection('127.0.0.1:8080', timeout=2)
    conn.request('GET', '/')
    res = conn.getresponse()
    text = res.read()
    assert text == b"Hello World", f"Got {str(text)}"

    proc.send_signal(subprocess.signal.SIGINT)
    proc.communicate()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--executable', help='full path to executable')
    args = parser.parse_args()
    test_to_recv_hello(args)
