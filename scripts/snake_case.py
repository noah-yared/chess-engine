#! /usr/bin/env python3

import os
import re
import sys


def pascal_or_camel_case_to_snake_case(filename: str) -> str:
    base, extension = os.path.splitext(filename)
    snake_case = re.sub(
        r"(?P<chunk>[A-Za-z][a-z]+)(?=[A-Z])", r"\g<chunk>_", base
    ).lower()
    return f"{snake_case}{extension}"


def filenames_to_snake_case(files: list[str]) -> None:
    assert all(re.fullmatch(file, "[A-Za-z]+") for file in files), "filenames must all be alphabetic!"
    abs_file_paths = [os.path.abspath(file) for file in files]
    for abs_file_path in abs_file_paths:
        os.rename(
            abs_file_path,
            os.path.join(
                os.path.dirname(abs_file_path),
                pascal_or_camel_case_to_snake_case(os.path.basename(abs_file_path)),
            ),
        )


def file_header_include_directives_to_snake_case(files: list[str]) -> None:
    def include_header_to_snake_case(match: re.Match[str]) -> str:
        snake_case_base = pascal_or_camel_case_to_snake_case(match.group("base"))
        return rf'#include "{snake_case_base}.{match.group("extension")}"'

    for file in files:
        with open(file, "r") as f:
            content = f.read()
        pattern = re.compile(r'#include\s+"(?P<base>[A-Za-z]+)\.(?P<extension>h|hpp)"')
        content_with_snake_case_headers = pattern.sub(
            include_header_to_snake_case, content
        )
        with open(file, "w") as f:
            f.write(content_with_snake_case_headers)


if __name__ == "__main__":
    file_header_include_directives_to_snake_case(sys.argv[1:])
    filenames_to_snake_case(sys.argv[1:])
