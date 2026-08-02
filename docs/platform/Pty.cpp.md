# Pty.cpp

`include/platform/Pty.cpp`

[← index](../README.md)

## Functions

- [`~Pty()`](#pty)
- [`open(const std::string& shell, int cols, int rows)`](#open)
- [`close()`](#close)
- [`read(std::string& out)`](#read)
- [`write(const char* data, std::size_t n)`](#write)
- [`resize(int cols, int rows)`](#resize)
- [`childExited()`](#childexited)

---

### ~Pty

```cpp
~Pty()
```

Closes the descriptor and reaps the child, so a destroyed terminal never leaves a shell running.

---

### open

```cpp
open(const std::string& shell, int cols, int rows)
```

**Parameters**

- `const std::string& shell`
- `int cols`
- `int rows`

**Returns** — bool -- false when no shell could be started

Forks a shell onto a new pseudo-terminal. The child is left entirely alone apart from its environment: TERM is set so the shell knows what escape sequences it may emit, and the window size is handed over up front so anything that queries it at startup gets the truth. The parent's end is put into non-blocking mode, which is what lets read() be pumped from a frame loop without ever stalling it. An empty shell name falls back to $SHELL and then to /bin/sh.

---

### close

```cpp
close()
```

**Returns** — void

Closes the descriptor, asks the child to quit and reaps it. The shell is given SIGHUP first, which is what it would get from a closing terminal, and SIGKILL only if it ignores that -- so a shell with unsaved state gets its usual chance to clean up.

---

### read

```cpp
read(std::string& out)
```

**Parameters**

- `std::string& out`

**Returns** — std::size_t -- bytes appended

Drains whatever the shell has written, appending to `out`. Never blocks: it reads until the descriptor is dry and returns, so it is safe to call once per frame. A read of 0 means the far end closed, which is how an exited shell is noticed. There is a cap per call so a program spewing output cannot starve the frame.

---

### write

```cpp
write(const char* data, std::size_t n)
```

**Parameters**

- `const char* data`
- `std::size_t n`

**Returns** — void

Sends bytes to the shell, retrying a partial or interrupted write. A full pipe is dropped rather than blocked on, since stalling the UI to feed a shell that is not reading would be worse than losing a keystroke.

---

### resize

```cpp
resize(int cols, int rows)
```

**Parameters**

- `int cols`
- `int rows`

**Returns** — void

Tells the shell the window changed size, which is what makes a running program redraw at the new dimensions. The kernel raises SIGWINCH in the child as a side effect of this ioctl.

---

### childExited

```cpp
childExited()
```

**Returns** — bool

Whether the shell has finished. Reaps it without blocking when it has, so an exited shell does not linger as a zombie.
