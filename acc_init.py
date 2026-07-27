#!/usr/bin/env python3
import os
import sys
import argparse

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from acc_common import init_runtime, AccError, fail


def main():
    parser = argparse.ArgumentParser(description="Initialize accumulator runtime")
    parser.add_argument("name")
    parser.add_argument("size", type=int)
    parser.add_argument("--force", action="store_true")
    args = parser.parse_args()

    try:
        init_runtime(args.name, args.size, args.force)
    except (AccError, OSError) as e:
        fail(str(e))

    print("ok init " + args.name + " " + str(args.size))


if __name__ == "__main__":
    main()
