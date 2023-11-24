#include <sys/util.h>
#include <graphx.h>
#include <sys/rtc.h>
#include <keypadc.h>
#include <sys/timers.h>
#include <fontlibc.h>

class Cell {
private:
    bool mine;
    bool uncovered;
    bool flagged;
    int val;
public:
    Cell() {
        mine = false;
        uncovered = false;
        flagged = false;
        val = 0;
    }
    void setMine() {
        mine = true;
    }
    bool isMine() {
        return mine;
    }
    void setVal(int n) {
        val = n;
    }
    int getVal() {
        return val;
    }
    bool isUncovered() {
        return uncovered;
    }
    void uncover() {
        uncovered = true;
        flagged = false;
    }
    void setUncovered(bool b) {
        uncovered = b;
    }
    bool isFlagged() {
        return flagged;
    }
    void flag() {
        flagged = !flagged;
    }
    void setFlag(bool f) {
        flagged = f;
    }
};

class Game {
public:
    int sizeX;
    int sizeY;
    int cellSize;
    int numMines;
    Cell** board;

    int cursorX;
    int cursorXo;
    int cursorYo;
    int cursorY;

    int numFlags;
    int numUncovered;
    bool generated;
    bool won;
    bool gameOver;

    Game(int difficulty) {
        switch(difficulty) {
        case 1:
            sizeX = 9;
            sizeY = 9;
            numMines = 10;
            break;
        case 2:
            sizeX = 16;
            sizeY = 16;
            numMines = 40;
            break;
        case 3:
            sizeX = 30;
            sizeY = 16;
            numMines = 99;
            break;
        }
        board = new Cell*[sizeY];
        for (int i = 0; i < sizeY; i++) {
            board[i] = new Cell[sizeX];
        }

        cellSize = 10;

        cursorX = sizeX / 2;
        cursorY = sizeY / 2;
        cursorXo = cursorX;
        cursorYo = cursorY;

        numFlags = 0;
        numUncovered = 0;
        won = false;
        gameOver = false;
        generated = false;
        timer_Disable(1);
        timer_Set(1, 0);
    }

    /*  Drawing Methods  */
    void drawScoreboard(int padding) {
        int sizeY = 16 + 2 * padding;
        gfx_SetColor(74);
        gfx_FillRectangle_NoClip(padding, padding, GFX_LCD_WIDTH - 2 * padding, sizeY);

        gfx_SetTextBGColor(0);
        gfx_SetTextFGColor(224);
        gfx_SetTextScale(2, 2);
        gfx_SetTextXY(2 * padding, 2 * padding);
        gfx_PrintInt(numMines - numFlags, 3);

        gfx_SetTextXY(GFX_LCD_WIDTH - 2 * padding - 48, 2 * padding);
        gfx_PrintInt(timer_Get(1) / 32768, 3);

        gfx_SetTextBGColor(255);
        gfx_SetTextScale(1, 1);
    }
    
    void showBoard() {
        int padding = 4;
        int xoffset = (GFX_LCD_WIDTH - sizeX * cellSize) / 2;
        int yoffset = (GFX_LCD_HEIGHT - (16+2*padding) - sizeY * cellSize) / 2 + (16+2*padding);
        gfx_FillScreen(222);
        drawScoreboard(padding);
        for (int r = 0; r < sizeY; r++) {
            for (int c = 0; c < sizeX; c++) {
                int x = xoffset + c * cellSize;
                int y = yoffset + r * cellSize;
                if (board[r][c].isUncovered()) {
                    if (!board[r][c].isMine() && !board[r][c].isFlagged()) {
                        int colors[] = {25, 4, 224, 9, 96, 12, 0, 74};
                        if (board[r][c].getVal() > 0 && board[r][c].getVal() < 9) {
                            gfx_SetTextFGColor(colors[board[r][c].getVal() - 1]);
                            gfx_SetTextXY(x+2, y+2);
                            gfx_PrintInt(board[r][c].getVal(), 1);
                        }
                    }
                    else if (board[r][c].isMine() && !board[r][c].isFlagged()) {
                        gfx_SetColor(224);
                        gfx_FillRectangle_NoClip(x + 1, y + 1, cellSize, cellSize);
                        gfx_SetTextFGColor(0);
                        gfx_PrintStringXY("M", x+2, y+2);
                    }
                    else if (board[r][c].isFlagged()) {
                        gfx_SetTextFGColor(0);
                        gfx_PrintStringXY("X", x+2, y+2);
                    }
                    gfx_SetColor(181);
                    gfx_Rectangle(x, y, cellSize + 1, cellSize + 1);
                }
                else {
                    gfx_SetColor(74);
                    gfx_Rectangle(x, y, cellSize, cellSize);
                    if (board[r][c].isFlagged()) {
                        gfx_SetTextFGColor(224);
                        gfx_PrintStringXY("F", x+2, y+2);
                    }
                }
            }
        }
        gfx_SetColor(0);
        gfx_Rectangle(xoffset + cursorX * cellSize, yoffset + cursorY * cellSize, cellSize + 1, cellSize + 1);
        gfx_Rectangle(xoffset + cursorX * cellSize - 1, yoffset + cursorY * cellSize - 1, cellSize + 3, cellSize + 3);
        gfx_SwapDraw();
    }

    /*  Generate new game  */
    void generateBoard() {
        for (int i = 0; i < numMines; i++) {
            bool safe = false;
            int x,y;
            while (!safe) {
                x = randInt(0, sizeX - 1);
                y = randInt(0, sizeY - 1);
                if ((abs(cursorX - x) > 1 or abs(cursorY - y) > 1) && !board[y][x].isMine())
                    safe = true;
            }
            board[y][x].setMine();
            for (int xn = x - 1; xn <= x + 1; xn++) {
                for (int yn = y - 1; yn <= y + 1; yn++) {
                    if (xn >= 0 && xn < sizeX && yn >= 0 && yn < sizeY && !board[yn][xn].isMine()) {
                        if (xn != x or yn != y) {
                            board[yn][xn].setVal(board[yn][xn].getVal() + 1);
                        }
                    }
                }
            }
        }
        generated = true;
    }

    /*  Input  */
    void move(int k) {
        if (k & kb_Left)
            if (cursorX > 0) cursorX--;
        if (k & kb_Right)
            if (cursorX < sizeX - 1) cursorX++;
        if (k & kb_Up)
            if (cursorY > 0) cursorY--;
        if (k & kb_Down)
            if (cursorY < sizeY - 1) cursorY++;
    }

    int act(int k) {
        switch (k) {
            case kb_2nd:
                if (!board[cursorY][cursorX].isUncovered()) {
                    board[cursorY][cursorX].flag();
                    numFlags = board[cursorY][cursorX].isFlagged() ? numFlags + 1 : numFlags - 1;
                }
                break;
            case kb_Mode:
                if (board[cursorY][cursorX].isFlagged()) return 0;
                if (board[cursorY][cursorX].isMine())
                    return -1;
                if (!generated) {
                    board[cursorY][cursorX].uncover();
                    generateBoard();
                    board[cursorY][cursorX].setUncovered(false);
                }
                return floodUncover(cursorX, cursorY);
        }
        return 0;
    }

    /*  Helper methods  */
    int floodUncover(int x, int y) {
        if (x < 0 || x >= sizeX || y < 0 || y >= sizeY)
            return 0;
        if (board[y][x].isUncovered())
            return 0;
        board[y][x].uncover();
        int numUncovered = 1;
        if (board[y][x].getVal() > 0 || board[y][x].isMine())
            return numUncovered;
        numUncovered += floodUncover(x-1, y-1);
        numUncovered += floodUncover(x, y-1);
        numUncovered += floodUncover(x+1, y-1);
        numUncovered += floodUncover(x-1, y);
        numUncovered += floodUncover(x+1, y);
        numUncovered += floodUncover(x-1, y+1);
        numUncovered += floodUncover(x, y+1);
        numUncovered += floodUncover(x+1, y+1);
        return numUncovered;
    }

    void lose() {
        for (int r = 0; r < sizeY; r++) {
            for (int c = 0; c < sizeX; c++) {
                if ((board[r][c].isMine() && !board[r][c].isFlagged()) || (board[r][c].isFlagged() && !board[r][c].isMine()))
                    board[r][c].setUncovered(true);
            }
        }
    }

    void win() {
        for (int r = 0; r < sizeY; r++) {
            for (int c = 0; c < sizeX; c++) {
                if (board[r][c].isMine()) {
                    board[r][c].setFlag(true);
                }
            }
        }
        numFlags = numMines;
    }

    /*  Main game loop  */
    void tick(kb_key_t &prevArrow, kb_key_t &prevAction) {
        if (numUncovered > 0 && timer_Get(1) == 0)
            timer_Enable(1, TIMER_32K, TIMER_NOINT, TIMER_UP);
        if (timer_Get(1) / 32768 >= 999)
            timer_Disable(1);

        kb_key_t arrows;
        kb_key_t actions;

        if (!gameOver) {
            arrows = kb_Data[7];
            if (arrows != prevArrow) {
                move(arrows);
                prevArrow = arrows;
            }
            actions = kb_Data[1];
            if (actions != prevAction) {
                int res = act(actions);
                if (res == -1) {
                    gameOver = true;
                    lose();
                }
                numUncovered += res;
                prevAction = actions;
            }
            
            cursorXo = cursorX;
            cursorYo = cursorY;
            
            if (numUncovered == sizeX * sizeY - numMines) {
                won = true;
                gameOver = true;
                win();
            }
        }
        else
            timer_Disable(1);
        showBoard();
    }
};

int mainMenu(kb_key_t &prev) {
    gfx_FillScreen(222);

    int padding = 4;
    int sizeY = 16 + 2 * padding;
    gfx_SetColor(74);
    gfx_FillRectangle_NoClip(padding, padding, GFX_LCD_WIDTH - 2 * padding, sizeY);

    gfx_SetTextBGColor(0);
    gfx_SetTextFGColor(224);
    gfx_SetTextScale(2, 2);
    gfx_PrintStringXY("MINESWEEPER", (GFX_LCD_WIDTH - 4 * padding - 16*11) / 2, 2 * padding);

    gfx_SetTextBGColor(255);
    gfx_SetTextFGColor(0);
    gfx_SetTextScale(1, 1);
    gfx_PrintStringXY("Beginner", (GFX_LCD_WIDTH - 4 * padding - 11 * 16) / 2, 4 * padding + 16);
    gfx_PrintStringXY("Intermediate", (GFX_LCD_WIDTH - 4 * padding - 11 * 16) / 2, 4 * padding + 16 + 12);
    gfx_PrintStringXY("Expert", (GFX_LCD_WIDTH - 4 * padding - 11 * 16) / 2, 4 * padding + 16 + 2 * 12);
    gfx_PrintStringXY("Mode: Select    2nd: Flag", (GFX_LCD_WIDTH - 8*25) / 2, GFX_LCD_HEIGHT - padding - 8);
    gfx_SwapDraw();
    gfx_SetDrawScreen();

    int difficulty = 1;
    kb_key_t key, prevArrow;

    do {
        kb_Scan();
        
        key = kb_Data[7];
        if (key != prevArrow) {
            switch (key) {
            case kb_Down:
                difficulty = (difficulty) % 3 + 1;
                break;
            case kb_Up:
                difficulty--;
                if (difficulty < 1) difficulty = 3;
                break;
            }
            prevArrow = key;
        }

        if (kb_Data[1] == kb_Mode && kb_Data[1] != prev) {
            prev = kb_Data[1];
            return difficulty;
        }

        gfx_SetTextFGColor(224);
        switch (difficulty) {
        case 1:
            gfx_PrintStringXY("Beginner", (GFX_LCD_WIDTH - 4 * padding - 11 * 16) / 2, 4 * padding + 16);
            gfx_SetTextFGColor(0);
            gfx_PrintStringXY("Intermediate", (GFX_LCD_WIDTH - 4 * padding - 11 * 16) / 2, 4 * padding + 16 + 12);
            gfx_PrintStringXY("Expert", (GFX_LCD_WIDTH - 4 * padding - 11 * 16) / 2, 4 * padding + 16 + 2 * 12);

            break;
        case 2:
            gfx_PrintStringXY("Intermediate", (GFX_LCD_WIDTH - 4 * padding - 11 * 16) / 2, 4 * padding + 16 + 12);
            gfx_SetTextFGColor(0);
            gfx_PrintStringXY("Beginner", (GFX_LCD_WIDTH - 4 * padding - 11 * 16) / 2, 4 * padding + 16);
            gfx_PrintStringXY("Expert", (GFX_LCD_WIDTH - 4 * padding - 11 * 16) / 2, 4 * padding + 16 + 2 * 12);
            break;
        case 3:
            gfx_PrintStringXY("Expert", (GFX_LCD_WIDTH - 4 * padding - 11 * 16) / 2, 4 * padding + 16 + 2 * 12);
            gfx_SetTextFGColor(0);
            gfx_PrintStringXY("Beginner", (GFX_LCD_WIDTH - 4 * padding - 11 * 16) / 2, 4 * padding + 16);
            gfx_PrintStringXY("Intermediate", (GFX_LCD_WIDTH - 4 * padding - 11 * 16) / 2, 4 * padding + 16 + 12);
            break;
        }     

    prev = kb_Data[1];
    } while (kb_Data[6] != kb_Clear);

    return -1;
}

bool game(int difficulty, kb_key_t &prevAction) {
    gfx_SetDrawBuffer();

    Game g(difficulty);
    g.showBoard();

    kb_key_t prevArrow;
    
    do {

        kb_Scan();
        g.tick(prevArrow, prevAction);

        if (kb_Data[1] == kb_Mode && kb_Data[1] != prevAction && g.gameOver) {
            prevAction = kb_Data[1];
            return true;
        }

        prevArrow = kb_Data[7];
        prevAction = kb_Data[1];
    } while (kb_Data[6] != kb_Clear);
    return false;
}

int main(void)
{
    srand(rtc_Time());

    gfx_Begin();
    
    kb_key_t prev;

    while (true) {
        int difficulty = mainMenu(prev);
        if (difficulty == -1) break;
        if (!game(difficulty, prev)) break;
    }

    /* End graphics drawing */
    gfx_End();

    return 0;
}
