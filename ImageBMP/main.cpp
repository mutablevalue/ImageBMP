#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include <cmath>
#include <algorithm>
#include "ImageBMP.h"

using namespace std;

class ExtendedImageBMP : public ImageBMP {
public:
    using ImageBMP::ImageBMP;

    void plotPixel(int x, int y, const Color& color) {
        // Check that (x, y) is in bounds.
        if (x < 0 || x >= static_cast<int>(infoHeader.imageWidth) ||
            y < 0 || y >= static_cast<int>(infoHeader.imageHeight))
        {
            return;
        }
        pixelData.pixelMatrix[y][x] = color;
    }

    // Draw a line from (x0, y0) to (x1, y1)
    void drawLine(int x0, int y0, int x1, int y1, const Color& color) {
        bool steep = abs(y1 - y0) > abs(x1 - x0);
        if (steep) {
            std::swap(x0, y0);
            std::swap(x1, y1);
        }
        if (x0 > x1) {
            std::swap(x0, x1);
            std::swap(y0, y1);
        }
        int dx = x1 - x0;
        int dy = abs(y1 - y0);
        int error = dx / 2;
        int y = y0;
        int ystep = (y0 < y1) ? 1 : -1;
        for (int x = x0; x <= x1; x++) {
            if (steep)
                // When steep, the algorithm’s (x,y) are swapped,
                // so we swap back 
                plotPixel(y, x, color);
            else
                plotPixel(x, y, color);
            error -= dy;
            if (error < 0) {
                y += ystep;
                error += dx;
            }
        }
    }

    // Draw a circle centered at (centerX, centerY)
    void drawCircle(int centerX, int centerY, int radius, const Color& color) {
        int x = radius;
        int y = 0;
        int decisionOver2 = 1 - x;
        while (y <= x) {
            // Plot the eight points.
            plotPixel(centerX + x, centerY + y, color);
            plotPixel(centerX + y, centerY + x, color);
            plotPixel(centerX - x, centerY + y, color);
            plotPixel(centerX - y, centerY + x, color);
            plotPixel(centerX - x, centerY - y, color);
            plotPixel(centerX - y, centerY - x, color);
            plotPixel(centerX + x, centerY - y, color);
            plotPixel(centerX + y, centerY - x, color);
            y++;
            if (decisionOver2 <= 0) {
                decisionOver2 += 2 * y + 1;
            }
            else {
                x--;
                decisionOver2 += 2 * (y - x) + 1;
            }
        }
    }

    // Draw a triangle
    void drawTriangle(int x0, int y0, int x1, int y1, int x2, int y2, const Color& color) {
        drawLine(x0, y0, x1, y1, color);
        drawLine(x1, y1, x2, y2, color);
        drawLine(x2, y2, x0, y0, color);
    }
};

// Returns true if the given player has three in a row.
bool checkWin(char board[3][3], char player) {
    for (int i = 0; i < 3; i++) {
         if (board[i][0] == player && board[i][1] == player && board[i][2] == player)
             return true;
         if (board[0][i] == player && board[1][i] == player && board[2][i] == player)
             return true;
    }
    if (board[0][0] == player && board[1][1] == player && board[2][2] == player)
         return true;
    if (board[0][2] == player && board[1][1] == player && board[2][0] == player)
         return true;
    return false;
}

// Draw the tic tac toe grid on the image.
void drawTicTacToeBoard(ExtendedImageBMP &img, int boardSize, const Color &lineColor) {
    int cellSize = boardSize / 3;
    // Draw vertical grid
    for (int i = 1; i < 3; i++) {
        int x = i * cellSize;
        img.drawLine(x, 0, x, boardSize, lineColor);
    }
    // Draw horizontal grid
    for (int i = 1; i < 3; i++) {
        int y = i * cellSize;
        img.drawLine(0, y, boardSize, y, lineColor);
    }
}

// Draw an "X"
void drawX(ExtendedImageBMP &img, int cellRow, int cellCol, int cellSize, const Color &color) {
    int margin = cellSize / 5;  // so X doesn’t touch the grid
    int x0 = cellCol * cellSize + margin;
    int y0 = cellRow * cellSize + margin;
    int x1 = (cellCol + 1) * cellSize - margin;
    int y1 = (cellRow + 1) * cellSize - margin;
    img.drawLine(x0, y0, x1, y1, color);
    img.drawLine(x0, y1, x1, y0, color);
}

// Draw an "O"
void drawO(ExtendedImageBMP &img, int cellRow, int cellCol, int cellSize, const Color &color) {
    int margin = cellSize / 5;
    int centerX = cellCol * cellSize + cellSize / 2;
    int centerY = cellRow * cellSize + cellSize / 2;
    int radius = (cellSize - 2 * margin) / 2;
    img.drawCircle(centerX, centerY, radius, color);
}

int main() {
    int choice;
    cout << "Select option:\n1. Drawing Shapes\n2. Tic Tac Toe Game\n";
    cin >> choice;
    
    if (choice == 1) {
        ExtendedImageBMP image(800, 600, Color(ColorEnum::White));
        image.drawLine(100, 100, 700, 100, Color(ColorEnum::Red));
        image.drawCircle(400, 300, 100, Color(ColorEnum::Blue));
        image.drawTriangle(200, 500, 400, 350, 600, 500, Color(ColorEnum::Green));
        image.writeImageFile("shapes.bmp");
        cout << "Shapes image has been saved\n";
    } else if (choice == 2) {
        //Tic Tac Toe Game
        const int boardSize = 600;
        ExtendedImageBMP boardImage(boardSize, boardSize, Color(ColorEnum::White));
        // Draw the grid
        drawTicTacToeBoard(boardImage, boardSize, Color(ColorEnum::Black));
        
        // Initialize board
        char board[3][3] = { {' ', ' ', ' '},
                             {' ', ' ', ' '},
                             {' ', ' ', ' '} };
        char currentPlayer = 'X';
        int moves = 0;
        
        while (true) {
            // Display the board state in the console:
            cout << "\nCurrent board:\n";
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    cout << board[i][j];
                    if (j < 2) cout << " | ";
                }
                cout << "\n";
                if (i < 2) cout << "---------\n";
            }
            int row, col;
            cout << "Player " << currentPlayer << ", enter row and column (0-2) Ex: 0 2, 1 2, 2 1, etc: ";
            cin >> row >> col;
            if (row < 0 || row > 2 || col < 0 || col > 2) {
                cout << "Invalid input, please try again.\n";
                continue;
            }
            if (board[row][col] != ' ') {
                cout << "Cell has been taken, please try again.\n";
                continue;
            }
            // Record the move in the board
            board[row][col] = currentPlayer;
            int cellSize = boardSize / 3;
            if (currentPlayer == 'X') {
                drawX(boardImage, row, col, cellSize, Color(ColorEnum::Red));
            } else {
                drawO(boardImage, row, col, cellSize, Color(ColorEnum::Blue));
            }
            moves++;
            if (checkWin(board, currentPlayer)) {
                cout << "Player " << currentPlayer << " wins!\n";
                break;
            }
            if (moves == 9) {
                cout << "It's a tie!\n";
                break;
            }
            // Switch players.
            currentPlayer = (currentPlayer == 'X') ? 'O' : 'X';
        }
        boardImage.writeImageFile("tictactoe.bmp");
        cout << "Tic Tac Toe board has been saved.\n";
    } else {
        cout << "Invalid option.\n";
    }
    return 0;
}
