import re
from pathlib import Path

from find_perfect_hasher import check_hash_function, hash_keys, MAP

LOG_FILE = Path(__file__).resolve().parent / "logs" / "perfect_hashers.out"

HASH_FUNC_PARAMS = 4

def parse_function_params(line: str) -> tuple[int, ...]:
    params = tuple(map(int, re.findall(r"\d+", line)))
    assert len(params) == HASH_FUNC_PARAMS, "Bad formatting in hashers log: perfect_hashers.out"
    return params

def is_correct_output(out: dict[str, int]) -> bool:
    return all(v == MAP[k] for k, v in out.items())

def check_file() -> tuple[bool, list[tuple[int, ...]]]:
    with open (LOG_FILE, "r") as log:
        params_list = [ 
            parse_function_params(line) 
            for line in log
        ]
    all_valid = all(success for success, _ in (check_hash_function(args) for args in params_list))
    bad_args = [args for args in params_list if not check_hash_function(args)[0]]
    return (
        all_valid, bad_args
    )

if __name__ == "__main__":

    good_hashers, bad_params = check_file()

    # all hashers are valid
    if good_hashers:
        print("All hashers working!")
    
    else:
        print("Some hashers are bad!")
        print("Bad hashers:")
        for bad_hasher in bad_params:
            print (f"{bad_hasher=}, hashed_keys={hash_keys(*bad_hasher)}")
    

