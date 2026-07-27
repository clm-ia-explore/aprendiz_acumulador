#!/usr/bin/env python3
import os
import sys
import math
import argparse

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from acc_common import (
    AccError,
    METHODS,
    read_runtime_values,
    normalize_values,
    fail,
)


def matrix_dims(n, allow_rect, cols_arg):
    side = math.isqrt(n)

    if side * side == n:
        return side, side

    if not allow_rect:
        raise AccError("n is not a perfect square; use --allow-rect")

    if cols_arg > 0:
        cols = cols_arg
    else:
        cols = side + 1

    if cols <= 0:
        raise AccError("bad cols")

    rows = (n + cols - 1) // cols
    return rows, cols


def print_matrix(values, rows, cols, formatter, pad=" ", sep=" "):
    n = len(values)

    for r in range(rows):
        cells = []
        for c in range(cols):
            i = r * cols + c
            if i < n:
                cells.append(formatter(values[i]))
            else:
                cells.append(pad)

        sys.stdout.write(sep.join(cells) + "\n")


def make_float_formatter(precision):
    def fmt(x):
        return "{:.{}f}".format(x, precision)
    return fmt


def clamp01_local(x):
    if x < 0.0:
        return 0.0
    if x > 1.0:
        return 1.0
    return float(x)


def heat_char(x, chars):
    x = clamp01_local(x)
    idx = int(x * (len(chars) - 1) + 0.5)
    if idx < 0:
        idx = 0
    if idx >= len(chars):
        idx = len(chars) - 1
    return chars[idx]


def display_values(raw, method, temperature, scale):
    if method == "raw":
        return [float(x) for x in raw]
    return normalize_values(raw, method, temperature, scale)


def print_single(raw, n, index, args):
    if index < 0 or index >= n:
        raise AccError("index out of range")

    if args.format == "heat":
        if args.method == "raw":
            disp = normalize_values(raw, "minmax", args.temperature, args.scale)
        else:
            disp = display_values(raw, args.method, args.temperature, args.scale)

        if len(args.heat_chars) < 2:
            raise AccError("heat-chars needs at least 2 chars")

        sys.stdout.write(heat_char(disp[index], args.heat_chars) + "\n")
        return

    if args.format == "int":
        if args.method == "raw":
            sys.stdout.write(str(int(raw[index])) + "\n")
        else:
            disp = display_values(raw, args.method, args.temperature, args.scale)
            x = clamp01_local(disp[index])
            sys.stdout.write(str(int(x * 100.0 + 0.5)) + "\n")
        return

    if args.method == "raw":
        x = float(raw[index])
    else:
        disp = display_values(raw, args.method, args.temperature, args.scale)
        x = disp[index]

    fmt = make_float_formatter(args.precision)
    sys.stdout.write(fmt(x) + "\n")


def main():
    parser = argparse.ArgumentParser(description="View accumulator matrix")
    parser.add_argument("name")
    parser.add_argument("--index", type=int, default=None)
    parser.add_argument("--method", default="minmax", choices=METHODS)
    parser.add_argument(
        "--format",
        default=None,
        choices=["float", "int", "heat", "csv"],
    )
    parser.add_argument("--temperature", type=float, default=1.0)
    parser.add_argument("--scale", type=float, default=1.0)
    parser.add_argument("--precision", type=int, default=4)
    parser.add_argument("--allow-rect", action="store_true")
    parser.add_argument("--cols", type=int, default=0)
    parser.add_argument("--heat-chars", default=".:-=+*#%@")
    parser.add_argument("--no-spaces", action="store_true")
    args = parser.parse_args()

    try:
        if args.precision < 0:
            raise AccError("precision must be >= 0")

        if args.format is None:
            if args.index is not None:
                args.format = "float"
            else:
                args.format = "int"

        n, raw = read_runtime_values(args.name)

        if args.index is not None:
            print_single(raw, n, args.index, args)
            return

        rows, cols = matrix_dims(n, args.allow_rect, args.cols)

        if args.format == "csv":
            disp = display_values(raw, args.method, args.temperature, args.scale)
            fmt = make_float_formatter(args.precision)

            for r in range(rows):
                cells = []
                for c in range(cols):
                    i = r * cols + c
                    if i < n:
                        cells.append(fmt(disp[i]))
                    else:
                        cells.append("")
                sys.stdout.write(",".join(cells) + "\n")

        elif args.format == "heat":
            if args.method == "raw":
                disp = normalize_values(raw, "minmax", args.temperature, args.scale)
            else:
                disp = display_values(raw, args.method, args.temperature, args.scale)

            chars = args.heat_chars
            if len(chars) < 2:
                raise AccError("heat-chars needs at least 2 chars")

            def fmt(x):
                return heat_char(x, chars)

            sep = "" if args.no_spaces else " "
            print_matrix(disp, rows, cols, fmt, pad=" ", sep=sep)

        elif args.format == "float":
            disp = display_values(raw, args.method, args.temperature, args.scale)
            fmt = make_float_formatter(args.precision)
            pad = " " * (args.precision + 4)
            print_matrix(disp, rows, cols, fmt, pad=pad, sep=" ")

        else:
            if args.method == "raw":
                disp = [int(x) for x in raw]
                width = max([len(str(x)) for x in disp] + [1])

                def fmt(x):
                    return str(x).rjust(width)

                pad = " " * width

            else:
                norm = normalize_values(
                    raw,
                    args.method,
                    args.temperature,
                    args.scale,
                )

                disp = []
                for x in norm:
                    x = clamp01_local(x)
                    disp.append(int(x * 100.0 + 0.5))

                def fmt(x):
                    return "{:3d}".format(x)

                pad = "   "

            print_matrix(disp, rows, cols, fmt, pad=pad, sep=" ")

    except (AccError, OSError) as e:
        fail(str(e))


if __name__ == "__main__":
    main()
