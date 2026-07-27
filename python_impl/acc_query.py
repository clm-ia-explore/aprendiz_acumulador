#!/usr/bin/env python3
import os
import sys
import argparse

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from acc_common import (
    AccError,
    METHODS,
    read_runtime_values,
    normalize_values,
    clamp01,
    fail,
)


def display_values(raw, method, temperature, scale):
    if method == "raw":
        return [float(x) for x in raw]
    return normalize_values(raw, method, temperature, scale)


def heat_index_from_float(x, chars):
    x = clamp01(x)
    idx = int(x * (len(chars) - 1) + 0.5)
    if idx < 0:
        idx = 0
    if idx >= len(chars):
        idx = len(chars) - 1
    return idx


def parse_float_threshold(text):
    try:
        v = float(text)
    except ValueError:
        raise AccError("threshold must be a float")
    if v != v:
        raise AccError("threshold must not be NaN")
    return v


def parse_int_threshold(text):
    try:
        return int(text)
    except ValueError:
        raise AccError("threshold must be an integer")


def parse_heat_threshold(text, chars):
    if len(chars) < 2:
        raise AccError("heat-chars needs at least 2 chars")

    if len(text) == 1:
        pos = chars.find(text)
        if pos != -1:
            return pos

    try:
        i = int(text)
        if 0 <= i < len(chars):
            return i
        raise AccError("heat threshold index out of range")
    except ValueError:
        pass

    try:
        f = float(text)
    except ValueError:
        raise AccError("heat threshold must be char, index, or 0..1 float")

    if f != f:
        raise AccError("threshold must not be NaN")
    if f < 0.0 or f > 1.0:
        raise AccError("heat threshold float must be in 0..1")

    return heat_index_from_float(f, chars)


def parse_threshold(text, args):
    if args.format == "heat":
        return parse_heat_threshold(text, args.heat_chars)
    if args.format == "int":
        return parse_int_threshold(text)
    return parse_float_threshold(text)


def build_items(raw, n, args):
    method_values = display_values(raw, args.method, args.temperature, args.scale)

    if args.format == "heat":
        if len(args.heat_chars) < 2:
            raise AccError("heat-chars needs at least 2 chars")

        if args.method == "raw":
            heat_values = normalize_values(raw, "minmax", args.temperature, args.scale)
        else:
            heat_values = method_values

        items = []
        chars = args.heat_chars
        for i in range(n):
            x = clamp01(heat_values[i])
            idx = heat_index_from_float(x, chars)
            items.append(
                {
                    "index": i,
                    "key": float(idx),
                    "tie": x,
                    "numeric": float(idx),
                    "value": chars[idx],
                }
            )
        return items

    if args.format == "int":
        items = []
        if args.method == "raw":
            for i in range(n):
                v = int(raw[i])
                items.append(
                    {
                        "index": i,
                        "key": float(v),
                        "tie": float(v),
                        "numeric": float(v),
                        "value": v,
                    }
                )
        else:
            for i in range(n):
                x = clamp01(method_values[i])
                v = int(x * 100.0 + 0.5)
                items.append(
                    {
                        "index": i,
                        "key": float(v),
                        "tie": x,
                        "numeric": float(v),
                        "value": v,
                    }
                )
        return items

    items = []
    for i in range(n):
        v = float(method_values[i])
        items.append(
            {
                "index": i,
                "key": v,
                "tie": v,
                "numeric": v,
                "value": v,
            }
        )
    return items


def format_value(item, fmt, precision):
    if fmt == "heat":
        return item["value"]
    if fmt == "int":
        return str(item["value"])
    return "{:.{}f}".format(item["value"], precision)


def main():
    parser = argparse.ArgumentParser(description="Query accumulator values")
    parser.add_argument("name")
    parser.add_argument("--method", default="minmax", choices=METHODS)
    parser.add_argument(
        "--format",
        default="float",
        choices=["float", "int", "heat", "csv"],
    )
    parser.add_argument("--min", dest="min_value", default=None)
    parser.add_argument("--max", dest="max_value", default=None)
    parser.add_argument("--k", "--limit", dest="k", type=int, default=None)
    parser.add_argument(
        "--order",
        default="auto",
        choices=["auto", "asc", "desc", "index"],
    )
    parser.add_argument("--temperature", type=float, default=1.0)
    parser.add_argument("--scale", type=float, default=1.0)
    parser.add_argument("--precision", type=int, default=4)
    parser.add_argument("--heat-chars", default=".:-=+*#%@")
    parser.add_argument("--values-only", action="store_true")
    parser.add_argument("--indices-only", action="store_true")
    parser.add_argument("--sep", default=None)
    args = parser.parse_args()

    try:
        if args.precision < 0:
            raise AccError("precision must be >= 0")
        if args.k is not None and args.k < 0:
            raise AccError("k must be >= 0")
        if args.values_only and args.indices_only:
            raise AccError("use only one of --values-only or --indices-only")

        min_val = None
        max_val = None

        if args.min_value is not None:
            min_val = parse_threshold(args.min_value, args)
        if args.max_value is not None:
            max_val = parse_threshold(args.max_value, args)

        if min_val is not None and max_val is not None and min_val > max_val:
            raise AccError("min greater than max")

        n, raw = read_runtime_values(args.name)
        items = build_items(raw, n, args)

        filtered = []
        for item in items:
            v = item["numeric"]
            if min_val is not None and v < min_val:
                continue
            if max_val is not None and v > max_val:
                continue
            filtered.append(item)

        if args.order == "index":
            filtered.sort(key=lambda it: it["index"])
        else:
            if args.order == "auto":
                if min_val is not None and max_val is None:
                    reverse = True
                elif max_val is not None and min_val is None:
                    reverse = False
                else:
                    reverse = True
            elif args.order == "asc":
                reverse = False
            else:
                reverse = True

            filtered = sorted(filtered, key=lambda it: it["index"])
            filtered = sorted(
                filtered,
                key=lambda it: (it["key"], it["tie"]),
                reverse=reverse,
            )

        if args.k is not None:
            filtered = filtered[: args.k]

        if args.sep is None:
            sep = "," if args.format == "csv" else " "
        else:
            sep = args.sep

        for item in filtered:
            if args.indices_only:
                sys.stdout.write(str(item["index"]) + "\n")
            elif args.values_only:
                sys.stdout.write(format_value(item, args.format, args.precision) + "\n")
            else:
                value = format_value(item, args.format, args.precision)
                sys.stdout.write(str(item["index"]) + sep + value + "\n")

    except UnicodeError:
        fail("unicode output error; try export LANG=C.UTF-8")
    except Exception as e:
        fail(str(e))


if __name__ == "__main__":
    main()
