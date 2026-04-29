#include "input_raw.h"
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

static struct termios orig;

void input_raw_init(void) {
    struct termios t;
    tcgetattr(STDIN_FILENO, &orig);
    t = orig;
    t.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &t);

    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
}

void input_raw_shutdown(void) {
    tcsetattr(STDIN_FILENO, TCSANOW, &orig);
}

int input_raw_read(unsigned char *buf, int max) {
    return read(STDIN_FILENO, buf, max);
}
