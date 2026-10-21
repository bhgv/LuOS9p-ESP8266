# Styx module

The module exposes firmware files, internal Lua objects and command callbacks
through its Styx/9P tree. The `luastyx` callbacks adapt these resources to the
`libstyx` server; `9infr` supplies the supporting Inferno C routines.

[Host Lua command regression and buffer contract](test/regression/README.md)
describes how to test result serialization and cached RPC reads without an
ESP8266. Run those commands from this directory. This does not replace the
firmware build or hardware tests.

Модуль публикует файлы, объекты Lua и командные обработчики через дерево Styx/9P.
По ссылке выше описаны проверки на компьютере без микроконтроллера и контракт
владения буферами. Сборка прошивки и испытания на ESP8266 выполняются отдельно.
