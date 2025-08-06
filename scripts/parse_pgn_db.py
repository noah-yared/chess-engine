import subprocess
from typing import IO

import chess.pgn

COMPRESSED_PGN_FILE_PATH = './data/lichess_db_standard_rated_2015-01.pgn.zst'
OUTPUT_FEN_FILE_PATH = './data/fen_data.txt'

def pipe_decompressed_pgn_zst(input_file_path: str) -> subprocess.Popen:
    with open(input_file_path, 'r') as input_file:
        return subprocess.Popen(['zstd', '-d', '-c', input_file], stdout=subprocess.PIPE, text=True, encoding='utf-8')

def process_pgn_data_to_fen_and_pipe(input_stream: IO[str], output_stream: IO[str], max_fens_to_process: int) -> None:
    def fen_after_move(board: chess.Board, move: chess.Move) -> str:
        board.push(move)
        return board.fen()

    written_fens = 0
    while written_fens < max_fens_to_process:
        game = chess.pgn.read_game(input_stream)
        if game is None: # eof
            break
        board = game.board()
        board_fens = [fen_after_move(board, move) for move in game.mainline_moves()]
        output_stream.write('\n'.join(board_fens) + '\n')
        written_fens += len(board_fens)

    output_stream.close()  # Signal EOF to output stream

def write_unique_fens_to_file_from_compressed_pgns(input_file_path: str, output_file_path: str, max_fens_to_process: int = 1_000_000, max_fens_to_write: int = 1_000_000) -> None:
    with open(output_file_path, 'w') as output_file:
        pgn_proc = pipe_decompressed_pgn_zst(input_file_path)
        pgn_proc.stdout.close() # unused end of pipe for parent process

        unique_fen_proc = subprocess.Popen(['sort', '-u'], stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True, encoding='utf-8')
        unique_fen_proc.stdout.close() # unused end of pipe for parent process

        truncate_fen_proc = subprocess.Popen(['head', '-n', str(max_fens_to_write)], stdin=unique_fen_proc.stdout, stdout=output_file, text=True, encoding='utf-8')

        process_pgn_data_to_fen_and_pipe(pgn_proc.stdout, unique_fen_proc.stdin, max_fens_to_process=max_fens_to_process)
        unique_fen_proc.stdin.close() # unnecessary, just for completeness (we close write end of pipe in process_pgn_data_to_fen_and_pipe)
        
        # Wait for processes to complete in reverse order to avoid deadlocks
        truncate_fen_proc.wait()
        unique_fen_proc.wait()
        pgn_proc.wait()


if __name__ == "__main__":
    write_unique_fens_to_file_from_compressed_pgns(COMPRESSED_PGN_FILE_PATH, OUTPUT_FEN_FILE_PATH)
