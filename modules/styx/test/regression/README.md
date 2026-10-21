# Lua command result regression

Run from `modules/styx` with Python 3, a C compiler and unpacked Lua sources
(the project's Lua 5.3.5 release works; point at its directory containing `lua.h`):

```sh
python3 test/regression/command_results.py --lua-source lua-5.3.5/src
python3 test/regression/command_results.py --lua-source lua-5.3.5/src --sanitize
```

`CC` and `CFLAGS` select the host compiler and flags. The test builds in a temporary
directory and does not download dependencies, listen on a port or run firmware.
Sanitizers require a host compiler/runtime supporting AddressSanitizer and UBSan.

The runner extracts the production `call_rpc`, `call_cgi` and `fsrpcread` functions
and compiles them with real Lua. Minimal Styx types isolate those callbacks from
the rest of the server. It checks multi-value and zero-value responses, numbers
and non-string values, embedded NUL bytes in the serializer, commands without a
trailing NUL, syntax/runtime errors, preservation of the caller's Lua stack,
response allocation failure, and reading a cached RPC response in multiple parts.
The old CGI allocation used the last value's size rather than the total. Both
callbacks passed `int *` where Lua writes a `size_t`, corrupting adjacent stack
storage on 64-bit hosts. The RPC reader also failed to save its completed state.

Each function owns `*pbuf`: on success it frees the command and installs a
NUL-terminated reply, returning its byte length (excluding that NUL). Existing tab
separators remain; values Lua cannot convert to strings produce empty fields.
Compilation and execution errors become response fields. An oversized reply or
an allocator that returns NULL produces an empty response in the existing buffer,
and restores the Lua stack. The public int allocator limits the complete
allocation, including the final NUL, to `INT_MAX`. Lua-state access remains
single-threaded or externally serialized. This is not a script sandbox.

This host regression does not validate the complete server, firmware, hardware
callbacks, networking, or arbitrary binary data through every file callback.
Some existing file readers still use `strlen`; the serializer's returned length
does not by itself make those readers binary-safe.

## По-русски

Тест запускается на компьютере с настоящим Lua, без микроконтроллера и сети.
Путь `--lua-source` должен указывать на каталог с `lua.h` и исходниками Lua.
Проверяются границы памяти, составные и пустые ответы, ошибки Lua, сохранение
стека, отказ выделения памяти и повторные чтения одного ответа RPC.
Тест не заменяет сборку прошивки и проверку полного сервера на устройстве.
