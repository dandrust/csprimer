# include <stdio.h>
# include <unistd.h>
# include <termios.h>
# include <stdlib.h>

/*
  disableRawMode() and enableRawMode() were inspired by
  the "Entering raw mode" chapter of "Build Your Own Text Editor"
  https://viewsourcecode.org/snaptoken/kilo/02.enteringRawMode.html
*/

struct termios orig_termios;

void disableRawMode() {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enableRawMode() {
  tcgetattr(STDIN_FILENO, &orig_termios);
  atexit(disableRawMode);

  struct termios raw = orig_termios;

  raw.c_lflag &= ~(ECHO | ICANON);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main () {
  int input, value, i;

  enableRawMode();

  while(1) {
    input = getc(stdin);

    if (input > 48 && input < 58) { 
      value = input - 48;

      for (i = 0; i < value; i++) {
        putc(0x07, stdout);
        fflush(stdout);
        sleep(1);
      }
    }
  }
}