# Pty.hpp

`include/platform/Pty.hpp`

[← index](../README.md)

## Types

- [Pty](#pty)

---

### Pty

A pseudo-terminal with a shell running on the far side of it. Owns the master file descriptor and the child process, reads without blocking so a UI can pump it once per frame, and writes keystrokes back. This is the whole OS-facing half of [Terminal](../elements/widgets/Terminal.hpp.md#terminal), kept separate so the widget itself has no platform code in it.

> POSIX is implemented with forkpty. Windows needs ConPTY, which is a different enough API to deserve its own file rather than an #ifdef ladder here; until then every call fails cleanly and [Terminal](../elements/widgets/Terminal.hpp.md#terminal) reports that it could not start a shell.

> Not copyable: it owns a file descriptor and a process.
