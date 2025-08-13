import itertools
from multiprocessing import Pool
import os
from pathlib import Path
from tqdm import tqdm
from time import perf_counter

KEYS = [ # modifiable
    "k", "q", "K", "Q",
]
VALS = [ # modifiable
    57, 61, 1, 5,
]
MAP = dict(zip(KEYS, VALS))

SCRIPT_DIR = Path(__file__).resolve().parent

LOG_DIR = SCRIPT_DIR / "logs"
OUT_FILE = LOG_DIR / "perfect_hashers3.out"


def input_gen(*args):
    A, B, C, D = args
    if A == B and A == C and A == D:
        return itertools.product(range(A), repeat=len(args))
    return itertools.product(range(A), range(B), range(C), range(D))


#  function: ((k^a) * b) & 63
def hash_keys(a, b, c, d, MOD=64):
    return [pow((((ord(k) * a) ^ b) >> c), d, MOD) for k in KEYS]


def check_hash_function(args):
    a, b, c, d = args
    if all(MAP[k] == v for k, v in zip(KEYS, hash_keys(a, b, c, d))):
        return True, (a, b, c, d)
    return False, None


def test(MUL_RANGE=64, XOR_RANGE=64, SFAMT_RANGE=64, PWR_RANGE=64, CHUNK_SIZE=10_000):
    with Pool() as pool, open(OUT_FILE, "w") as log:
        for success, args in tqdm(
            pool.imap_unordered(
                check_hash_function,
                input_gen(MUL_RANGE, XOR_RANGE, SFAMT_RANGE, PWR_RANGE),
                CHUNK_SIZE,
            ),
            total=MUL_RANGE * XOR_RANGE * SFAMT_RANGE * PWR_RANGE,
            desc="Testing hash functions",
        ):
            if success:
                print("FOUND A GOOD TRIPLE!")
                print(args, file=log)  # write working function params to log file


if __name__ == "__main__":
    LOG_DIR.mkdir(exist_ok=True)

    if os.path.exists(OUT_FILE):
        OUT_FILE.unlink()
        print(f"Deleted file: {OUT_FILE}")

    start = perf_counter()
    test()
    end = perf_counter()

    print(f"\nTests took {end - start}s")
