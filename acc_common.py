import os
import re
import sys
import struct
import fcntl
import mmap
import array
import zlib
import math
import json

VERSION = 1

MAGIC_SHM = b"ACCSHM01"
MAGIC_DAT = b"ACCDAT01"
MAGIC_QDAT = b"ACCQDAT1"

SHM_HEADER_FMT = "<8sIIII"
DAT_HEADER_FMT = "<8sIIII"
QDAT_HEADER_FMT = "<8sIIIiiI"

SHM_HEADER_SIZE = struct.calcsize(SHM_HEADER_FMT)
DAT_HEADER_SIZE = struct.calcsize(DAT_HEADER_FMT)
QDAT_HEADER_SIZE = struct.calcsize(QDAT_HEADER_FMT)

VALUES_OFFSET = SHM_HEADER_SIZE

INT64_MIN = -(2 ** 63)
INT64_MAX = 2 ** 63 - 1
INT32_MIN = -(2 ** 31)
INT32_MAX = 2 ** 31 - 1

METHODS = (
    "raw",
    "minmax",
    "posmax",
    "signed01",
    "sum01",
    "softmax",
    "sigmoid",
    "tanh01",
    "rank",
)

METHOD_CODES = {name: i for i, name in enumerate(METHODS)}
CODE_METHODS = {i: name for i, name in enumerate(METHODS)}

_NAME_RE = re.compile(r"^[A-Za-z0-9._-]+$")


class AccError(Exception):
    pass


def fail(msg):
    sys.stderr.write("error: " + msg + "\n")
    sys.exit(1)


def validate_name(name):
    if not _NAME_RE.match(name):
        raise AccError("invalid name: use A-Z a-z 0-9 . _ -")
    return name


def shm_dir():
    return os.environ.get("ACC_SHM_DIR", "/dev/shm")


def shm_bin_path(name):
    validate_name(name)
    d = shm_dir()
    os.makedirs(d, exist_ok=True)
    return os.path.join(d, "acc_" + name + ".bin")


def shm_lock_path(name):
    validate_name(name)
    d = shm_dir()
    os.makedirs(d, exist_ok=True)
    return os.path.join(d, "acc_" + name + ".lock")


def shm_meta_path(name):
    """Path for metadata cache file associated with a runtime."""
    validate_name(name)
    d = shm_dir()
    os.makedirs(d, exist_ok=True)
    return os.path.join(d, "acc_" + name + ".meta")


class Lock:
    def __init__(self, name, shared=False):
        self.lock_path = shm_lock_path(name)
        self.shared = shared
        self.fd = None

    def __enter__(self):
        self.fd = os.open(self.lock_path, os.O_RDWR | os.O_CREAT, 0o644)
        op = fcntl.LOCK_SH if self.shared else fcntl.LOCK_EX
        fcntl.flock(self.fd, op)
        return self

    def __exit__(self, exc_type, exc_value, tb):
        fcntl.flock(self.fd, fcntl.LOCK_UN)
        os.close(self.fd)
        self.fd = None
        return False


def read_metadata(name):
    """Read cached metadata for a runtime. Returns dict or None if not found/invalid."""
    meta_path = shm_meta_path(name)
    try:
        with open(meta_path, "r") as f:
            data = json.load(f)
        if isinstance(data, dict) and "n" in data and "mtime" in data:
            bin_path = shm_bin_path(name)
            if os.path.exists(bin_path):
                current_mtime = os.path.getmtime(bin_path)
                if data.get("mtime") == current_mtime:
                    return data
        return None
    except (OSError, json.JSONDecodeError, KeyError):
        return None


def write_metadata(name, n):
    """Write cached metadata for a runtime."""
    meta_path = shm_meta_path(name)
    bin_path = shm_bin_path(name)
    try:
        mtime = os.path.getmtime(bin_path)
        data = {"n": n, "mtime": mtime}
        tmp = meta_path + ".tmp"
        with open(tmp, "w") as f:
            json.dump(data, f)
            f.flush()
            os.fsync(f.fileno())
        os.replace(tmp, meta_path)
    except OSError:
        pass


def clamp_int64(x):
    if x < INT64_MIN:
        return INT64_MIN
    if x > INT64_MAX:
        return INT64_MAX
    return int(x)


def array_to_little_bytes(values):
    a = array.array("q", values)
    if sys.byteorder != "little":
        a.byteswap()
    return a.tobytes()


def little_bytes_to_array(data):
    a = array.array("q")
    a.frombytes(data)
    if sys.byteorder != "little":
        a.byteswap()
    return a


def read_runtime_header(f):
    f.seek(0)
    data = f.read(SHM_HEADER_SIZE)
    if len(data) != SHM_HEADER_SIZE:
        raise AccError("bad runtime header")

    magic, version, n, flags, _ = struct.unpack(SHM_HEADER_FMT, data)

    if magic != MAGIC_SHM:
        raise AccError("bad runtime magic")
    if version != VERSION:
        raise AccError("bad runtime version")
    if flags != 0:
        raise AccError("unsupported runtime flags")
    if n <= 0:
        raise AccError("bad runtime size")

    return n


def write_runtime_file(bin_path, n, values_little_bytes=None):
    if n <= 0:
        raise AccError("bad runtime size")
    if n > 0xFFFFFFFF:
        raise AccError("runtime size too large")

    header = struct.pack(SHM_HEADER_FMT, MAGIC_SHM, VERSION, n, 0, 0)
    tmp = bin_path + ".tmp"

    with open(tmp, "wb") as f:
        f.write(header)

        if values_little_bytes is None:
            remaining = 8 * n
            chunk = b"\x00" * 1048576
            while remaining > 0:
                if remaining >= len(chunk):
                    f.write(chunk)
                    remaining -= len(chunk)
                else:
                    f.write(chunk[:remaining])
                    remaining = 0
        else:
            if len(values_little_bytes) != 8 * n:
                raise AccError("bad values length")
            f.write(values_little_bytes)

        f.flush()
        os.fsync(f.fileno())

    os.replace(tmp, bin_path)


def init_runtime(name, n, force=False):
    validate_name(name)

    if n <= 0:
        raise AccError("size must be positive")
    if n > 0xFFFFFFFF:
        raise AccError("size too large")

    bin_path = shm_bin_path(name)

    with Lock(name, shared=False):
        if os.path.exists(bin_path) and not force:
            raise AccError("runtime already exists; use --force")
        write_runtime_file(bin_path, n, None)
        write_metadata(name, n)


def apply_stimulus(name, index, value):
    validate_name(name)
    bin_path = shm_bin_path(name)

    with Lock(name, shared=False):
        try:
            f = open(bin_path, "r+b")
        except FileNotFoundError:
            raise AccError("runtime not initialized")

        try:
            meta = read_metadata(name)
            if meta is not None:
                n = meta["n"]
            else:
                n = read_runtime_header(f)

            if index < 0 or index >= n:
                raise AccError("index out of range")

            st = os.fstat(f.fileno())
            expected = VALUES_OFFSET + 8 * n
            if st.st_size < expected:
                raise AccError("bad runtime file length")

            mm = mmap.mmap(f.fileno(), 0)
            try:
                offset = VALUES_OFFSET + 8 * index
                old = struct.unpack_from("<q", mm, offset)[0]
                new = clamp_int64(old + value)
                struct.pack_into("<q", mm, offset, new)
                mm.flush()
                return new
            finally:
                mm.close()
        finally:
            f.close()


def read_runtime_values(name):
    validate_name(name)
    bin_path = shm_bin_path(name)

    with Lock(name, shared=True):
        try:
            f = open(bin_path, "rb")
        except FileNotFoundError:
            raise AccError("runtime not initialized")

        try:
            meta = read_metadata(name)
            if meta is not None:
                n = meta["n"]
            else:
                n = read_runtime_header(f)

            st = os.fstat(f.fileno())
            expected = VALUES_OFFSET + 8 * n
            if st.st_size < expected:
                raise AccError("bad runtime file length")

            mm = mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ)
            try:
                end = VALUES_OFFSET + 8 * n
                data = mm[VALUES_OFFSET:end]
            finally:
                mm.close()
        finally:
            f.close()

    if len(data) != 8 * n:
        raise AccError("bad runtime data length")

    values = little_bytes_to_array(data)
    return n, values


def memorize_runtime(name, file_path, force=False):
    validate_name(name)

    if os.path.exists(file_path) and not force:
        raise AccError("output file exists; use --force")

    n, values = read_runtime_values(name)
    values_bytes = array_to_little_bytes(values)
    crc = zlib.crc32(values_bytes) & 0xFFFFFFFF

    header = struct.pack(DAT_HEADER_FMT, MAGIC_DAT, VERSION, n, 0, crc)
    tmp = file_path + ".tmp"

    with open(tmp, "wb") as f:
        f.write(header)
        f.write(values_bytes)
        f.flush()
        os.fsync(f.fileno())

    os.replace(tmp, file_path)


def read_dat_file(file_path):
    with open(file_path, "rb") as f:
        header = f.read(DAT_HEADER_SIZE)
        if len(header) != DAT_HEADER_SIZE:
            raise AccError("bad dat header")

        magic, version, n, flags, crc = struct.unpack(DAT_HEADER_FMT, header)

        if magic != MAGIC_DAT:
            raise AccError("bad dat magic")
        if version != VERSION:
            raise AccError("bad dat version")
        if flags != 0:
            raise AccError("unsupported dat flags")
        if n <= 0:
            raise AccError("bad dat size")

        data = f.read(8 * n)
        if len(data) != 8 * n:
            raise AccError("bad dat data length")

    calc = zlib.crc32(data) & 0xFFFFFFFF
    if calc != crc:
        raise AccError("bad dat crc")

    values = little_bytes_to_array(data)
    return n, values


def reconstruct_runtime(name, file_path, force=False):
    validate_name(name)

    n, values = read_dat_file(file_path)
    values_bytes = array_to_little_bytes(values)
    bin_path = shm_bin_path(name)

    with Lock(name, shared=False):
        if os.path.exists(bin_path) and not force:
            raise AccError("runtime already exists; use --force")
        write_runtime_file(bin_path, n, values_bytes)
        write_metadata(name, n)


def clamp01(x):
    if x < 0.0:
        return 0.0
    if x > 1.0:
        return 1.0
    return float(x)


def normalize_values(values, method, temperature=1.0, scale=1.0):
    if method not in METHOD_CODES:
        raise AccError("unknown method")

    vals = [float(x) for x in values]
    n = len(vals)

    if n == 0:
        return []

    if method == "raw":
        return vals

    if method == "minmax":
        mn = min(vals)
        mx = max(vals)
        if mx == mn:
            return [0.0] * n
        rng = mx - mn
        return [(x - mn) / rng for x in vals]

    if method == "posmax":
        pos = [x if x > 0.0 else 0.0 for x in vals]
        mx = max(pos)
        if mx <= 0.0:
            return [0.0] * n
        return [x / mx for x in pos]

    if method == "signed01":
        mx = max(abs(x) for x in vals)
        if mx <= 0.0:
            return [0.5] * n
        return [(x / mx + 1.0) / 2.0 for x in vals]

    if method == "sum01":
        mn = min(vals)
        shifted = [x - mn for x in vals]
        s = sum(shifted)
        if s <= 0.0:
            return [1.0 / n] * n
        return [x / s for x in shifted]

    if method == "softmax":
        if temperature <= 0.0:
            raise AccError("temperature must be positive")
        mx = max(vals)
        exps = [math.exp((x - mx) / temperature) for x in vals]
        s = sum(exps)
        if s <= 0.0:
            return [1.0 / n] * n
        return [e / s for e in exps]

    if method == "sigmoid":
        if scale <= 0.0:
            raise AccError("scale must be positive")

        out = []
        for x in vals:
            z = x / scale
            if z >= 0.0:
                e = math.exp(-z)
                out.append(1.0 / (1.0 + e))
            else:
                e = math.exp(z)
                out.append(e / (1.0 + e))
        return out

    if method == "tanh01":
        if scale <= 0.0:
            raise AccError("scale must be positive")
        return [(math.tanh(x / scale) + 1.0) / 2.0 for x in vals]

    if method == "rank":
        if n == 1:
            return [0.0]

        pairs = sorted((vals[i], i) for i in range(n))
        ranks = [0.0] * n

        i = 0
        while i < n:
            j = i
            while j + 1 < n and pairs[j + 1][0] == pairs[i][0]:
                j += 1

            avg = (i + j) / 2.0
            for k in range(i, j + 1):
                ranks[pairs[k][1]] = avg

            i = j + 1

        return [r / (n - 1) for r in ranks]

    raise AccError("unknown method")


def quantize_floats(floats, qmin, qmax, renorm_max=False):
    if qmin > qmax:
        raise AccError("min must be <= max")
    if qmin < INT32_MIN or qmax > INT32_MAX:
        raise AccError("min/max out of int32 range")

    vals = list(floats)

    if renorm_max:
        mx = max(vals) if vals else 0.0
        if mx > 0.0:
            vals = [x / mx for x in vals]
        else:
            vals = [0.0 for _ in vals]

    rng = qmax - qmin
    out = []

    for x in vals:
        x = clamp01(x)
        q = int(math.floor(qmin + x * rng + 0.5))
        if q < qmin:
            q = qmin
        if q > qmax:
            q = qmax
        out.append(q)

    return out


def int32_array_to_little_bytes(qvalues):
    try:
        a = array.array("i", qvalues)
    except OverflowError:
        raise AccError("quantized value out of range")

    if a.itemsize != 4:
        raise AccError("platform int array is not 32 bit")

    if sys.byteorder != "little":
        a.byteswap()

    return a.tobytes()


def write_qdat_file(output_path, n, method, qmin, qmax, qvalues, force=False):
    if method not in METHOD_CODES:
        raise AccError("unknown method")

    if qmin < INT32_MIN or qmax > INT32_MAX:
        raise AccError("min/max out of int32 range")

    if len(qvalues) != n:
        raise AccError("bad qvalues length")

    if os.path.exists(output_path) and not force:
        raise AccError("output file exists; use --force")

    data = int32_array_to_little_bytes(qvalues)
    crc = zlib.crc32(data) & 0xFFFFFFFF

    header = struct.pack(
        QDAT_HEADER_FMT,
        MAGIC_QDAT,
        VERSION,
        n,
        METHOD_CODES[method],
        qmin,
        qmax,
        crc,
    )

    tmp = output_path + ".tmp"

    with open(tmp, "wb") as f:
        f.write(header)
        f.write(data)
        f.flush()
        os.fsync(f.fileno())

    os.replace(tmp, output_path)
