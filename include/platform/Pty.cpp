#include "Pty.hpp"

#include <cstdio>
#include <cstring>

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

/* iOS has forkpty in its headers but a sandboxed app may not spawn processes,
   so a terminal cannot work there at all -- it fails the same clean way the
   unfinished Windows backend does rather than crashing at runtime. */
#if defined(__APPLE__) && TARGET_OS_IPHONE
#define UILO_PTY_UNSUPPORTED 1
#elif defined(_WIN32)
#define UILO_PTY_UNSUPPORTED 1
#endif

#if defined(UILO_PTY_UNSUPPORTED)
/* ConPTY is a different API shape and belongs in its own file. Until that
   exists every call fails cleanly, so Terminal reports "could not start a
   shell" rather than the build failing or the widget half-working. */
namespace uilo {

Pty::~Pty() {}

bool Pty::open(const std::string&, int, int) {
#if defined(__APPLE__) && TARGET_OS_IPHONE
    m_error = "pty: iOS does not allow an app to spawn a shell";
#else
    m_error = "pty: not implemented on Windows (needs ConPTY)";
#endif
    return false;
}
void        Pty::close()                            {}
std::size_t Pty::read(std::string&)                 { return 0; }
void        Pty::write(const char*, std::size_t)    {}
void        Pty::resize(int, int)                   {}
bool        Pty::childExited()                      { return true; }

}

#else

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#if defined(__APPLE__)
#  include <util.h>
#else
#  include <pty.h>
#endif

namespace uilo {

/*
    ~Pty():
    - Params:   none
    - Returns:  none
    - Desc:     Closes the descriptor and reaps the child, so a destroyed
                terminal never leaves a shell running.
*/
Pty::~Pty() { close(); }


/*
    open(const std::string& shell, int cols, int rows):
    - Params:   const std::string& shell, int cols, int rows
    - Returns:  bool -- false when no shell could be started
    - Desc:     Forks a shell onto a new pseudo-terminal. The child is left
                entirely alone apart from its environment: TERM is set so the
                shell knows what escape sequences it may emit, and the window
                size is handed over up front so anything that queries it at
                startup gets the truth. The parent's end is put into
                non-blocking mode, which is what lets read() be pumped from a
                frame loop without ever stalling it. An empty shell name falls
                back to $SHELL and then to /bin/sh.
*/
bool Pty::open(const std::string& shell, int cols, int rows) {
    if (isOpen()) return true;

    std::string path = shell;
    if (path.empty()) {
        if (const char* env = ::getenv("SHELL")) path = env;
    }
    if (path.empty()) path = "/bin/sh";

    struct winsize ws {};
    ws.ws_col = static_cast<unsigned short>(cols > 0 ? cols : 80);
    ws.ws_row = static_cast<unsigned short>(rows > 0 ? rows : 24);

    int   master = -1;
    pid_t pid    = ::forkpty(&master, nullptr, nullptr, &ws);

    if (pid < 0) {
        m_error = std::string("pty: forkpty failed: ") + std::strerror(errno);
        return false;
    }

    if (pid == 0) {
        /* Child. TERM has to be something the shell recognises or it will fall
           back to the dumbest possible output. */
        ::setenv("TERM", "xterm-256color", 1);
        ::unsetenv("LINES");
        ::unsetenv("COLUMNS");

        const char* argv0 = path.c_str();
        if (const char* slash = std::strrchr(argv0, '/')) argv0 = slash + 1;

        /* A leading '-' asks for a login shell, which is what gives the user
           their usual prompt and aliases. */
        std::string dashed = "-" + std::string(argv0);
        ::execl(path.c_str(), dashed.c_str(), (char*)nullptr);
        ::execl("/bin/sh", "-sh", (char*)nullptr);
        ::_exit(127);
    }

    m_fd     = master;
    m_pid    = static_cast<long>(pid);
    m_exited = false;
    m_error.clear();

    const int flags = ::fcntl(m_fd, F_GETFL, 0);
    ::fcntl(m_fd, F_SETFL, (flags < 0 ? 0 : flags) | O_NONBLOCK);
    return true;
}


/*
    close():
    - Params:   none
    - Returns:  void
    - Desc:     Closes the descriptor, asks the child to quit and reaps it. The
                shell is given SIGHUP first, which is what it would get from a
                closing terminal, and SIGKILL only if it ignores that -- so a
                shell with unsaved state gets its usual chance to clean up.
*/
void Pty::close() {
    if (m_fd >= 0) { ::close(m_fd); m_fd = -1; }

    if (m_pid > 0) {
        const pid_t pid = static_cast<pid_t>(m_pid);
        ::kill(pid, SIGHUP);

        int status = 0;
        for (int i = 0; i < 50; ++i) {
            const pid_t r = ::waitpid(pid, &status, WNOHANG);
            if (r == pid || r < 0) { m_pid = -1; break; }
            ::usleep(2000);
        }
        if (m_pid > 0) {
            ::kill(pid, SIGKILL);
            ::waitpid(pid, &status, 0);
            m_pid = -1;
        }
    }
    m_exited = true;
}


/*
    read(std::string& out):
    - Params:   std::string& out
    - Returns:  std::size_t -- bytes appended
    - Desc:     Drains whatever the shell has written, appending to `out`. Never
                blocks: it reads until the descriptor is dry and returns, so it
                is safe to call once per frame. A read of 0 means the far end
                closed, which is how an exited shell is noticed. There is a cap
                per call so a program spewing output cannot starve the frame.
*/
std::size_t Pty::read(std::string& out) {
    if (m_fd < 0) return 0;

    constexpr std::size_t kChunk  = 4096;
    constexpr std::size_t kMaxPer = 1u << 20;   /* 1 MB per pump */

    char        buf[kChunk];
    std::size_t total = 0;

    while (total < kMaxPer) {
        const ssize_t n = ::read(m_fd, buf, sizeof(buf));
        if (n > 0) {
            out.append(buf, static_cast<std::size_t>(n));
            total += static_cast<std::size_t>(n);
            continue;
        }
        if (n == 0) { m_exited = true; break; }
        if (errno == EINTR) continue;
        /* EAGAIN is the normal "nothing more right now"; EIO is what a closed
           pty master reports on this platform when the child is gone. */
        if (errno != EAGAIN && errno != EWOULDBLOCK) m_exited = true;
        break;
    }
    return total;
}


/*
    write(const char* data, std::size_t n):
    - Params:   const char* data, std::size_t n
    - Returns:  void
    - Desc:     Sends bytes to the shell, retrying a partial or interrupted
                write. A full pipe is dropped rather than blocked on, since
                stalling the UI to feed a shell that is not reading would be
                worse than losing a keystroke.
*/
void Pty::write(const char* data, std::size_t n) {
    if (m_fd < 0 || !data || n == 0) return;

    std::size_t off = 0;
    while (off < n) {
        const ssize_t w = ::write(m_fd, data + off, n - off);
        if (w > 0) { off += static_cast<std::size_t>(w); continue; }
        if (w < 0 && errno == EINTR) continue;
        break;
    }
}


/*
    resize(int cols, int rows):
    - Params:   int cols, int rows
    - Returns:  void
    - Desc:     Tells the shell the window changed size, which is what makes a
                running program redraw at the new dimensions. The kernel raises
                SIGWINCH in the child as a side effect of this ioctl.
*/
void Pty::resize(int cols, int rows) {
    if (m_fd < 0 || cols <= 0 || rows <= 0) return;

    struct winsize ws {};
    ws.ws_col = static_cast<unsigned short>(cols);
    ws.ws_row = static_cast<unsigned short>(rows);
    ::ioctl(m_fd, TIOCSWINSZ, &ws);
}


/*
    childExited():
    - Params:   none
    - Returns:  bool
    - Desc:     Whether the shell has finished. Reaps it without blocking when
                it has, so an exited shell does not linger as a zombie.
*/
bool Pty::childExited() {
    if (m_exited) return true;
    if (m_pid <= 0) return true;

    int         status = 0;
    const pid_t r      = ::waitpid(static_cast<pid_t>(m_pid), &status, WNOHANG);
    if (r == static_cast<pid_t>(m_pid) || r < 0) {
        m_exited = true;
        m_pid    = -1;
    }
    return m_exited;
}

}

#endif
