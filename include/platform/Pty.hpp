#pragma once

#include <cstddef>
#include <string>

namespace uilo {

/*
    Pty:
    - Desc: A pseudo-terminal with a shell running on the far side of it. Owns
            the master file descriptor and the child process, reads without
            blocking so a UI can pump it once per frame, and writes keystrokes
            back. This is the whole OS-facing half of Terminal, kept separate so
            the widget itself has no platform code in it.
    - POSIX is implemented with forkpty. Windows needs ConPTY, which is a
      different enough API to deserve its own file rather than an #ifdef ladder
      here; until then every call fails cleanly and Terminal reports that it
      could not start a shell.
    - Not copyable: it owns a file descriptor and a process.
*/
class Pty {
public:
    Pty() = default;
    ~Pty();

    Pty(const Pty&)            = delete;
    Pty& operator=(const Pty&) = delete;

    bool open(const std::string& shell, int cols, int rows);
    void close();

    bool isOpen() const { return m_fd >= 0; }

    std::size_t read(std::string& out);
    void        write(const char* data, std::size_t n);
    void        write(const std::string& s) { write(s.data(), s.size()); }

    void resize(int cols, int rows);
    bool childExited();

    const std::string& lastError() const { return m_error; }

private:
    int         m_fd     = -1;
    long        m_pid    = -1;
    bool        m_exited = false;
    std::string m_error;
};

}
