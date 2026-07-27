#!/usr/bin/env python3
import os
import sys
import argparse

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from acc_common import apply_stimulus, AccError, fail


def main():
    parser = argparse.ArgumentParser(description="Apply one stimulus")
    parser.add_argument("name")
    parser.add_argument("index", type=int)
    parser.add_argument("value", type=int)
    parser.add_argument("--quiet", action="store_true")
    args = parser.parse_args()

    try:
        new_value = apply_stimulus(args.name, args.index, args.value)
    except (AccError, OSError) as e:
        fail(str(e))

    if not args.quiet:
        print("ok stim " + args.name + " " + str(args.index) + " " + str(new_value))


if __name__ == "__main__":
    main()
