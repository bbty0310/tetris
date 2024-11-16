// key_input.h
#ifndef KEY_INPUT_H
#define KEY_INPUT_H

#include <stdio.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

void setup_terminal(struct termio *original, struct termio *modified);
void restore_terminal(struct termio *original);
void read_input();

#endif // KEY_INPUT_H
