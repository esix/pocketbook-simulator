//------------------------------------------------------------------
// game.h
//
// Author:           JuanJakobo
// Date:             19.4.2020
// Description:      Contains the tik-tak-toe game logic
//-------------------------------------------------------------------

#ifndef GAME
#define GAME

#include "field.h"
#include <vector>
#include <string>

using namespace std;

class Game {
    public:
        Game();
        int getMove();
        void nextMove();
        void drawBoard(int boardBeginX, int boardBeginY, int boardWidth, int boardHeight);
        bool doMove(int x, int y);
        string whosTurn();
        bool gameOver();
        bool checkForWinner();

    private:
        vector<Field> boardfields;
        int yFieldCount;
        int xFieldCount;
        int fieldsCount;
        int move;
        int fieldHeight;
};

#endif
