import subprocess
import argparse


def _run(args, threads_no):
    return subprocess.Popen([args.executable, str(threads_no)],
                            stdout=subprocess.PIPE,
                            stdin=subprocess.PIPE,
                            stderr=subprocess.PIPE)


def test_with_one_number(args, threads_no):
    proc = _run(args, threads_no)
    out, _ = proc.communicate(input=b"1 0")
    assert out == b"0\n"


def test_with_distinct_numbers(args, threads_no):
    proc = _run(args, threads_no)
    out, _ = proc.communicate(input=b"10 2 7 9 6 4 5 0 8 1 3")
    assert out == b"0 1 2 3 4 5 6 7 8 9\n"


def test_with_asc_numbers(args, threads_no):
    proc = _run(args, threads_no)
    out, _ = proc.communicate(input=b"10 0 1 2 3 4 5 6 7 8 9")
    assert out == b"0 1 2 3 4 5 6 7 8 9\n"


def test_with_desc_numbers(args, threads_no):
    proc = _run(args, threads_no)
    out, _ = proc.communicate(input=b"10 9 8 7 6 5 4 3 2 1 0")
    assert out == b"0 1 2 3 4 5 6 7 8 9\n"


def test_with_repeated_numbers_1(args, threads_no):
    proc = _run(args, threads_no)
    out, _ = proc.communicate(input=b"10 9 0 7 3 5 4 3 1 1 0")
    assert out == b"0 0 1 1 3 3 4 5 7 9\n"


def test_with_repeated_numbers_2(args, threads_no):
    proc = _run(args, threads_no)
    out, _ = proc.communicate(input=b"11 7 4 5 9 1 0 3 3 0 1 7")
    assert out == b"0 0 1 1 3 3 4 5 7 7 9\n"


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--executable', help='full path to executable')
    args = parser.parse_args()

    for i in (1, 2, 3, 8, 100):
        test_with_one_number(args, i)
        test_with_distinct_numbers(args, i)
        test_with_asc_numbers(args, i)
        test_with_desc_numbers(args, i)
        test_with_repeated_numbers_1(args, i)
        test_with_repeated_numbers_2(args, i)
