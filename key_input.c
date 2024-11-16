// key_input.c
#include "key_input.h"

void setup_terminal(struct termio *original, struct termio *modified) {
    ioctl(0, TCGETA, original);
    *modified = *original;

    modified->c_lflag &= ~(ECHO | ICANON);
    modified->c_cc[VMIN] = 0;
    modified->c_cc[VTIME] = 1;
    ioctl(0, TCSETAF, modified);
}

void restore_terminal(struct termio *original) {
    ioctl(0, TCSETAF, original);
}

void read_input() {
    char in_char = 0; 
    char read_byte = 0; 
    struct termio tty_backup; 
    struct termio tty_change;

    setup_terminal(&tty_backup, &tty_change);

    while(in_char != 0x0a) {
        read_byte = read(0, &in_char, 1);
        switch(read_byte) {
            case -1:
                restore_terminal(&tty_backup);
                return;
            case 0:
                break; 
            default:
                putchar(in_char);
                fflush(NULL);
        }
    }

    restore_terminal(&tty_backup);
}
