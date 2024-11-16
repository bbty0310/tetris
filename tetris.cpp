#include <iostream>
#include <ctime>
#include <cstdlib>
#include <algorithm>
#include <vector>
#include <array>
#include <thread>
#include <chrono>
#include <cstdlib> 
#include <atomic>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "key_input.h"
using namespace std;

const int BOARD_WIDTH = 10;
const int BOARD_HEIGHT = 20;
const int Pieces_Size = 4;

enum PiecesType { I, O, T, J, L, S, Z };

struct Pieces {
    array<array<int, Pieces_Size>, Pieces_Size> pieces;
    PiecesType type;

    Pieces(PiecesType type) : type(type) {
        switch(type) {
            case I:
                pieces = {{
                    {0,0,1,0},
                    {0,0,1,0},
                    {0,0,1,0},
                    {0,0,1,0},
                }};
                break;
            case O:
                pieces = {{
                    {1,1,0,0},
                    {1,1,0,0},
                    {0,0,0,0},
                    {0,0,0,0},
                }};
                break;
            case T:
                pieces = {{
                    {1,1,1,0},
                    {0,1,0,0},
                    {0,0,0,0},
                    {0,0,0,0},
                }};
                break;
            case J:
                pieces = {{
                    {1,0,0,0},
                    {1,1,1,0},
                    {0,0,0,0},
                    {0,0,0,0},
                }};
                break;
            case L:
                pieces = {{
                    {0,0,1,0},
                    {1,1,1,0},
                    {0,0,0,0},
                    {0,0,0,0},
                }};
                break;
            case S:
                pieces = {{
                    {0,1,1,0},
                    {1,1,0,0},
                    {0,0,0,0},
                    {0,0,0,0},
                }};
                break;
            case Z:
                pieces = {{
                    {1,1,0,0},
                    {0,1,1,0},
                    {0,0,0,0},
                    {0,0,0,0},
                }};
                break;
        }
    }
};

class Tetris {
private:
    int score;
    int level;
    int board[BOARD_HEIGHT][BOARD_WIDTH];

    Pieces pieces;
    int x, y;
    int dropInterval;
    atomic<bool> running;
    atomic<bool> blockMoved;

    struct termios original_termios;

public:
    Tetris() : pieces(static_cast<PiecesType>(rand() % 7)), x(BOARD_WIDTH / 2 - 2), y(0), score(0), level(1), dropInterval(1000) {
        initializeMap();
        createMap();
        setupTerminal();
    }

    ~Tetris() {
        restoreTerminal();
    }

    void initializeMap();
    void createMap();
    void drawBoard();
    void start();
    bool movePieceDown();
    void placePiece();
    void newPiece();
    int khbit();
    bool checkGameOver();
    void handleInput();
    void moveLeft();
    void moveRight();
    void rotatePiece();
    void updateScore(int clear);
    void updateLevel();
    void showGameOver();
    void clearScreen();

    // 터미널 settings
    void setupTerminal();
    void restoreTerminal();
};

void Tetris::initializeMap() {
    for (int i = 0; i < BOARD_HEIGHT; i++) {
        for (int j = 0; j < BOARD_WIDTH; j++) {
            board[i][j] = 0;
        }
    }
}

void Tetris::createMap() {
    for (int i = 0; i < BOARD_HEIGHT; i++) {
        board[i][0] = 2;  // 왼쪽 벽
        board[i][BOARD_WIDTH-1] = 2;  // 오른쪽 벽
    }
    for (int j = 0; j < BOARD_WIDTH; j++) {
        board[BOARD_HEIGHT-1][j] = 2;  // 바닥
    }
}

void Tetris::drawBoard() {
    clearScreen();  
    cout << "Score: " << score << " Level: " << level << endl;
    for (int i = 0; i < BOARD_HEIGHT; i++) {
        for (int j = 0; j < BOARD_WIDTH; j++) {
            if (i >= y && i < y + Pieces_Size && j >= x && j < x + Pieces_Size && pieces.pieces[i-y][j-x] == 1) {
                cout << "▣ ";
            } else if (board[i][j] == 1) {
                cout << "▣ ";
            } else if (board[i][j] == 2) {
                cout << "■ ";
            } else {
                cout << "□ ";
            }
        }
        cout << endl;
    }
}

void Tetris::clearScreen() {
    cout << "\033[2J\033[1;1H";
}

bool Tetris::movePieceDown() {
    for (int i = 0; i < Pieces_Size; ++i) {
        for (int j = 0; j < Pieces_Size; ++j) {
            if (pieces.pieces[i][j] == 1) {
                int newY = y + i + 1;
                if (newY >= BOARD_HEIGHT || board[newY][x + j] != 0) {
                    placePiece();
                    return false;
                }
            }
        }
    }

    y++;
    blockMoved = true;
    return true;
}

void Tetris::placePiece() {
    for (int i = 0; i < Pieces_Size; ++i) {
        for (int j = 0; j < Pieces_Size; ++j) {
            if (pieces.pieces[i][j] == 1) {
                board[y + i][x + j] = 1;
            }
        }
    }

    int linesCleared = 0;
    for (int i = BOARD_HEIGHT - 2; i >= 0; --i) {
        bool fullLine = true;
        for (int j = 1; j < BOARD_WIDTH - 1; ++j) {
            if (board[i][j] == 0) {
                fullLine = false;
                break;
            }
        }

        if (fullLine) {
            linesCleared++;
            for (int k = i; k > 0; --k) {
                for (int j = 1; j < BOARD_WIDTH - 1; ++j) {
                    board[k][j] = board[k - 1][j];
                }
            }
            for (int j = 1; j < BOARD_WIDTH - 1; ++j) {
                board[0][j] = 0;
            }
            i++;
        }
    }

    updateScore(linesCleared);
    updateLevel();
    newPiece();
}

int kbhit() {
    struct termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if(ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }

    return 0;
}

void Tetris::newPiece() {
    pieces = Pieces(static_cast<PiecesType>(rand() % 7));
    x = BOARD_WIDTH / 2 - 2;
    y = 0;
}

void Tetris::moveLeft() {
    for (int i = 0; i < Pieces_Size; ++i) {
        for (int j = 0; j < Pieces_Size; ++j) {
            if (pieces.pieces[i][j] == 1) {
                if (x + j - 1 < 1 || board[y + i][x + j - 1] != 0) return;
            }
        }
    }
    x--;
}

void Tetris::moveRight() {
    for (int i = 0; i < Pieces_Size; ++i) {
        for (int j = 0; j < Pieces_Size; ++j) {
            if (pieces.pieces[i][j] == 1) {
                if (x + j + 1 >= BOARD_WIDTH - 1 || board[y + i][x + j + 1] != 0) return;
            }
        }
    }
    x++;
}

void Tetris::rotatePiece() {
    array<array<int, Pieces_Size>, Pieces_Size> rotated = {};
    for (int i = 0; i < Pieces_Size; i++) {
        for (int j = 0; j < Pieces_Size; j++) {
            rotated[j][Pieces_Size-1-i] = pieces.pieces[i][j];
        }
    }

    for (int i = 0; i < Pieces_Size; ++i) {
        for (int j = 0; j < Pieces_Size; ++j) {
            if (rotated[i][j] == 1) {
                if (y + i >= BOARD_HEIGHT || x + j < 1 || x + j >= BOARD_WIDTH - 1 || board[y + i][x + j] != 0) return;
            }
        }
    }

    pieces.pieces = rotated;
}

void Tetris::updateScore(int clear) {
    switch(clear) {
        case 1: score += 100; break;
        case 2: score += 300; break;
        case 3: score += 500; break;
        case 4: score += 800; break;
    }
}

void Tetris::updateLevel() {
    level = min(score / 1000 + 1, 10);
    dropInterval = max(100, 1000 - level * 100);
}

void Tetris::showGameOver() {
    clearScreen();
    cout << "Game Over!" << endl;
    cout << "Final Score: " << score << endl;
    cout << "Final Level: " << level << endl;
}

atomic<bool> blockMoved(false);

void Tetris::start() {
    thread inputThread(&Tetris::handleInput, this);

    while (running) {
        auto start = chrono::steady_clock::now();

        if (!blockMoved) movePieceDown(); 

        drawBoard();
        blockMoved = false; 

        this_thread::sleep_until(start + chrono::milliseconds(dropInterval)); 
    }

    inputThread.join();
    showGameOver();
}

bool Tetris::checkGameOver() {
    for (int i = 0; i < Pieces_Size; ++i) {
        for (int j = 0; j < Pieces_Size; ++j) {
            if (pieces.pieces[i][j] == 1 && board[y + i][x + j] != 0) return true;
        }
    }
    return false;
}

void Tetris::handleInput() {
    while (running) {
        char input;
        if (read(STDIN_FILENO, &input, 1) == -1) continue;

        switch(input) {
            case 'a': moveLeft(); break;
            case 'd': moveRight(); break;
            case 's': movePieceDown(); break;
            case 'w': rotatePiece(); break;
            case 'q': running = false; break;
        }

        blockMoved = true;
    }
}


void Tetris::setupTerminal() {
    struct termios new_termios;
    tcgetattr(STDIN_FILENO, &original_termios);
    new_termios = original_termios;
    new_termios.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
}

void Tetris::restoreTerminal() {
    tcsetattr(STDIN_FILENO, TCSANOW, &original_termios);
}

int main() {
    srand(time(0));
    Tetris tetris;
    tetris.start();
    return 0;
}
