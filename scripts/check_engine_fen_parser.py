from multiprocessing import Pool, Manager
from multiprocessing.managers import AcquirerProxy
import subprocess
import os
from pathlib import Path
from time import perf_counter

# get directory of script for reference
SCRIPT_DIR = Path(__file__).resolve().parent

MAX_FEN_BATCH_SIZE = 1_000

ENGINE_BUILD_PATH = SCRIPT_DIR.parent / "build"
ENGINE_EXE = ENGINE_BUILD_PATH / "engine"

LOG_DIR = SCRIPT_DIR / "logs"
LOG_FILE = LOG_DIR / "parsed_fens.log"

if os.path.exists(LOG_FILE):
    LOG_FILE.unlink()
    print(f"Deleted file: {LOG_FILE}")

def write_to_file(s: str) -> None:
    with open(LOG_FILE, "w") as log:
        log.write(s)              

###########################################################
################# _____  ___  ____   ___  #################
#################   |   |   | |   \ |   | #################
#################   |   |   | |   | |   | #################
#################   |   |___| |___/ |___| #################
#################                         #################
###########################################################

if __name__ == "__main__":
    pass
