#!/usr/bin/env python3
"""Compile the two production command functions, without firmware dependencies."""
import argparse
import os
from pathlib import Path
import shlex
import subprocess
import tempfile


def command_function(path, name, result="int"):
    text = path.read_text()
    start = text.index(result + "\n" + name + "(")
    # Both entry points end at a column-zero closing brace.
    end = text.index("\n}", start) + 2
    return text[start:end]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--lua-source", required=True, type=Path,
                        help="Lua source directory containing lua.h and lapi.c")
    parser.add_argument("--sanitize", action="store_true")
    parser.add_argument("--source-dir", type=Path,
                        default=Path(__file__).resolve().parents[2] / "luastyx")
    args = parser.parse_args()
    compiler = shlex.split(os.environ.get("CC", "cc"))
    flags = shlex.split(os.environ.get("CFLAGS", "-O1 -g"))
    if args.sanitize:
        flags += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
    lua = args.lua_source.resolve()
    sources = [str(p) for p in sorted(lua.glob("l*.c"))
               if p.name not in ("lua.c", "luac.c", "ltests.c")]
    if not (lua / "lua.h").is_file() or not sources:
        parser.error("--lua-source must point to unpacked Lua C sources")
    with tempfile.TemporaryDirectory(prefix="styx-results-") as temporary:
        root = Path(temporary)
        unit = root / "commands.c"
        unit.write_text(
            '#include <limits.h>\n#include <string.h>\n'
            '#include "lua.h"\n#include "lauxlib.h"\n'
            '#include "command_types.h"\n'
            'extern lua_State *intL;\nvoid *styxmalloc(int);\n'
            'void styxfree(void *);\n#define DBG(...)\n' +
            "\n".join(command_function(args.source_dir / ("lstyx_" + n + ".c"),
                                        "call_" + n) for n in ("rpc", "cgi")) +
            "\n" + command_function(args.source_dir / "lstyx_rpc.c",
                                     "fsrpcread", "char *"))
        executable = root / "command-results"
        subprocess.run(compiler + flags + ["-I" + str(lua),
                       "-I" + str(Path(__file__).resolve().parent), str(unit),
                       str(Path(__file__).with_suffix(".c"))] + sources +
                       ["-lm", "-o", str(executable)], check=True)
        subprocess.run([str(executable)], check=True)
    print("PASS real-Lua command result lengths, bounds, errors and stack")


if __name__ == "__main__":
    main()
