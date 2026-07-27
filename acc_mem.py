#!/usr/bin/env python3
import os
import sys
import argparse

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from acc_common import (
    memorize_runtime,
    reconstruct_runtime,
    AccError,
    fail,
)


def main():
    parser = argparse.ArgumentParser(
        description="Memorize or reconstruct accumulator state"
    )

    sub = parser.add_subparsers(dest="action")
    sub.required = True

    p_mem = sub.add_parser("memorize")
    p_mem.add_argument("name")
    p_mem.add_argument("file")
    p_mem.add_argument("--force", action="store_true")

    p_rec = sub.add_parser("reconstruct")
    p_rec.add_argument("name")
    p_rec.add_argument("file")
    p_rec.add_argument("--force", action="store_true")

    args = parser.parse_args()

    try:
        if args.action == "memorize":
            memorize_runtime(args.name, args.file, args.force)
        else:
            reconstruct_runtime(args.name, args.file, args.force)
    except (AccError, OSError) as e:
        fail(str(e))

    print("ok " + args.action + " " + args.name + " " + args.file)


if __name__ == "__main__":
    main()
