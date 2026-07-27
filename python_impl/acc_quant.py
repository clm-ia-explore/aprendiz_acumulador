#!/usr/bin/env python3
import os
import sys
import argparse

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from acc_common import (
    AccError,
    METHODS,
    read_dat_file,
    normalize_values,
    quantize_floats,
    write_qdat_file,
    fail,
)


def main():
    parser = argparse.ArgumentParser(description="Quantize normalized accumulator data")
    parser.add_argument("input")
    parser.add_argument("output")
    parser.add_argument("--method", default="minmax", choices=METHODS)
    parser.add_argument("--min", dest="qmin", type=int, default=0)
    parser.add_argument("--max", dest="qmax", type=int, default=1000)
    parser.add_argument("--temperature", type=float, default=1.0)
    parser.add_argument("--scale", type=float, default=1.0)
    parser.add_argument("--renorm-max", action="store_true")
    parser.add_argument("--allow-raw", action="store_true")
    parser.add_argument("--force", action="store_true")
    args = parser.parse_args()

    try:
        if args.method == "raw" and not args.allow_raw:
            raise AccError("raw method is disabled for quantization; use --allow-raw")

        n, values = read_dat_file(args.input)

        floats = normalize_values(
            values,
            args.method,
            args.temperature,
            args.scale,
        )

        qvalues = quantize_floats(
            floats,
            args.qmin,
            args.qmax,
            args.renorm_max,
        )

        write_qdat_file(
            args.output,
            n,
            args.method,
            args.qmin,
            args.qmax,
            qvalues,
            args.force,
        )

    except (AccError, OSError) as e:
        fail(str(e))

    print("ok quant " + args.input + " " + args.output + " " + args.method)


if __name__ == "__main__":
    main()
