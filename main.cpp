/*
    Author: Jan Rudnicki
    Program: Jumping Frog
    Version: 1.0
*/

#include <curses.h>
#include <stdlib.h>
#include <time.h>

//**********************
//* DEFINING CONSTANTS *
//**********************

#define NUMROWS LINES * 2 / 3 // DEFINING BOARD SIZE PARAMETERS
#define NUMCOLS (COLS / 4)

#define MINIMUM NUMROWS/2 // DEFINING MINIMUM AND MAXIMUM NUMBER OF ROADS
#define MAXIMUM NUMROWS-2

#define END_COLOR 1 // DEFINING COLORS
#define FREE_COLOR 2 // DEFINE THE COLOR OF THE FREE AREA
#define OBSTACLE_COLOR 3 
#define START_COLOR 4 // DEFINE THE COLOR OF THE STARTING AREA
#define FROG_COLOR 5
#define FRIENDLY_COLOR 6 // COLOR OF A FRIENDLY CAR THAT IS TURNED ON
#define STORK_COLOR 7

//***********************
//* DEFINING STRUCTURES *
//***********************

struct Car {
    int x; // Car's position
    int y;
    int direction; // 0 for left, 1 for right
    char symbol;
    bool stop_now; // Boolean to check wherer the car should stop now
    int speed;
    clock_t last_move_time; // Time of the last move
};

struct Road {
    int x; // Road's row
    Car car; // Car on the road
};

struct FreeRow { // Define a row on which there is no road
    bool is_free;
    bool* obstacles;
};

struct Board {
    int rows; // Board's size
    int cols;
    char** grid; // Grid of the board
    Road* roads; // Array of roads
    FreeRow* free_rows;
    int num_roads; // Number of roads
    int car_min_speed; // Minimum and maximum speed of the cars
    int car_max_speed;
    int spaces_count; // The number of spaces clicked in order to decide if the friendly car should be on/off
    bool friendly_on; // Boolean to check wheter the friednly cars shoudl be on
};

struct Object { // define the object that is frog
    int x; // Object's position
    int y;
    char symbol;
    int speed; // Object's speed
    clock_t last_move_time; // Time of the last move
    int last_key; // Last key pressed
    bool on_friendly_car; // Boolean to check wheter the frog is on the friendly car
    int friendly_car_index; // Index of the friendly car
    int level; // the level in which the frog is currently in
    int lanes_passed;
};

struct Stork { //Creating a stork that will chase the frog
    int x;
    int y;
    char symbol;
    int speed;
    clock_t last_move_time;
};

struct Timer {
    clock_t start_time;
    double current_time;
};

//*************************
//* COLORS INITIALIZATION *
//*************************

void createColorPairs() {
    start_color();
    init_pair(END_COLOR, COLOR_GREEN, COLOR_GREEN);
    init_pair(START_COLOR, COLOR_RED, COLOR_RED);
    init_pair(FREE_COLOR, COLOR_YELLOW, COLOR_WHITE);
    init_pair(OBSTACLE_COLOR, COLOR_BLACK, COLOR_WHITE);
    init_pair(FROG_COLOR, COLOR_BLUE, COLOR_BLUE);
    init_pair(FRIENDLY_COLOR, COLOR_WHITE, COLOR_GREEN);
    init_pair(STORK_COLOR, COLOR_WHITE, COLOR_RED);
}

//****************************
//* FREE THE MEMORY FUNCTION *
//****************************

void freeMemory(Board* board, Object* frog, Timer* timer, Stork* stork) {
    for (int i = 0; i < board->rows; i++) {
        delete[] board->grid[i];
        if (i != 0 && i != board->rows - 1) {
            delete[] board->free_rows[i].obstacles;
        }
    }
    delete[] board->grid;
    delete[] board->roads;
    delete[] board->free_rows;
    delete board;
    delete frog;
    delete timer;
    delete stork;
}

//***************************
//* TIMER RELATED FUNCTIONS *
//***************************

void initTimer(Timer* timer) {
    timer->start_time = clock();
    timer->current_time = 0.00;
}

void updateTimer(Timer* timer) {
    timer->current_time = double(clock() - timer->start_time) / CLOCKS_PER_SEC; // getting the current time in seconds
}

void showTimer(Timer* timer) {
    mvprintw(NUMROWS / 2, NUMCOLS + 1, "Time: %.2f s", timer->current_time);
}

//***************************
//* BOARD RELATED FUNCTIONS *
//***************************

// Function to initialize the grid of the board
void initGrid(Board* board) {
    board->grid = new char* [board->rows];
    for (int i = 0; i < board->rows; i++) {
        board->grid[i] = new char[board->cols];
        for (int j = 0; j < board->cols - 1; j++) {
            board->grid[i][j] = ' ';
        }
        board->grid[i][board->cols - 1] = '\0';
    }
}

// Find a free row for the road
void findRow(Board* board, int i) {
    board->roads[i].x = (rand() % (NUMROWS - 2)) + 1; // Random road position
    bool is_occupied = false;
    do {
        is_occupied = false;
        for (int j = 0; j < i; j++) {
            if (board->roads[i].x == board->roads[j].x) {
                is_occupied = true;
                break;
            }
        }
        if (is_occupied) {
            board->roads[i].x++;
            if (board->roads[i].x >= NUMROWS - 1) {
                board->roads[i].x = 1;
            }
        }
    } while (is_occupied);
}

// Function to initialize the car
void initCar(Board* board, int i) {
    board->roads[i].car.x = board->roads[i].x;
    int left_right = rand() % 2; // The car is placed randomly on left or right edge of the row
    if (left_right == 0) { // Car spawns on the left edge and is moving to right edge
        board->roads[i].car.direction = 1;
        board->roads[i].car.y = 0;
    }
    else { // Car spawns on the right edge and is moving to left edge
        board->roads[i].car.direction = -1;
        board->roads[i].car.y = board->cols - 2;
    }
    int which_symbol = rand() % 3;
    if (which_symbol == 0) {
        board->roads[i].car.symbol = 'C';
    }
    else if (which_symbol == 1) {
        board->roads[i].car.symbol = 'S';
    }
    else {
        board->roads[i].car.symbol = 'F';
    }
    board->roads[i].car.stop_now = false;
    board->roads[i].car.speed = board->car_min_speed + rand() % (board->car_max_speed - board->car_min_speed + 1); // Random speed of the car
    board->roads[i].car.last_move_time = clock();
}

// Function to initialize the roads of the board
void initRoads(Board* board) {
    board->num_roads = (MINIMUM + rand() % ((MAXIMUM - MINIMUM) + 1)); // Random number of roads
    board->roads = new Road[board->num_roads];
    for (int i = 0; i < board->num_roads; i++) {
        findRow(board, i); // Calling a function to find a free row for the road
        board->free_rows[board->roads[i].x].is_free = false; // Marking the row as occupied
        initCar(board, i); // Calling a function to initialize the car
    }
}

// Function to initialize the board
void initBoard(Board* board, int MINSPEED, int MAXSPEED) {
    board->rows = NUMROWS;
    board->cols = NUMCOLS;
    board->free_rows = new FreeRow[board->rows];
    for (int i = 0; i < board->rows; i++) {
        if (i == 0 || i == board->rows - 1) {
            board->free_rows[i].is_free = false;
        }
        else {
            board->free_rows[i].is_free = true;
            board->free_rows[i].obstacles = new bool[board->cols - 1];
            int distance_between_obstacles = rand() % 5 + 1;
            if (distance_between_obstacles != 1) {
                for (int j = 0; j < board->cols - 1; j++) {
                    if (j % distance_between_obstacles == 0) {
                        board->free_rows[i].obstacles[j] = true;
                    }
                    else {
                        board->free_rows[i].obstacles[j] = false;
                    }
                }
            }
            else {
                for (int j = 0; j < board->cols - 1; j++) {
                    board->free_rows[i].obstacles[j] = false;
                }
            }
        }
    }
    board->car_min_speed = MINSPEED;
    board->car_max_speed = MAXSPEED;
    board->spaces_count = 0;
    board->friendly_on = false;
    initGrid(board);
    initRoads(board);
}

// Function to print the grid of the board
void printGrid(Board* board) {
    for (int i = 0; i < board->rows; i++) {
        //deciding the color of the row
        if (i == 0) {
            attron(COLOR_PAIR(END_COLOR));
            mvprintw(i, 0, board->grid[i]);
            attroff(COLOR_PAIR(END_COLOR));
        }
        else if (i == board->rows - 1) {
            attron(COLOR_PAIR(START_COLOR));
            mvprintw(i, 0, board->grid[i]);
            attroff(COLOR_PAIR(START_COLOR));
        }
        else {
            if (board->free_rows[i].is_free) {
                for (int j = 0; j < board->cols - 1; j++) {
                    if (board->free_rows[i].obstacles[j]) {
                        attron(COLOR_PAIR(OBSTACLE_COLOR));
                        mvaddch(i, j, 'X');
                        attroff(COLOR_PAIR(OBSTACLE_COLOR));
                    }
                    else {
                        attron(COLOR_PAIR(FREE_COLOR));
                        mvaddch(i, j, board->grid[i][j]);
                        attroff(COLOR_PAIR(FREE_COLOR));
                    }
                }
            }
        }
    }
}

// Function to print the roads of the board
void printRoads(Board* board) {
    for (int i = 0; i < board->num_roads; i++) {
        for (int j = 0; j < NUMCOLS - 1; j++) {
            mvaddch(board->roads[i].x, j, '-');
        }
    }
}

// Function to print the board
void printBoard(Board* board) {
    printGrid(board);
    printRoads(board);
}

//*************************
//* CAR RELATED FUNCTIONS *
//*************************

// Changing the friendly cars on demand
void FriendlyOnOff(Board* board) {
    board->spaces_count++;
    for (int i = 0; i < board->num_roads; i++) {
        if (board->roads[i].car.symbol == 'F') {
            if (board->spaces_count % 2 == 0) {
                board->friendly_on = false;
            }
            else {
                board->friendly_on = true;
            }
        }
    }
}

bool disappearCar(Board* board, int i) {
    int disappear = rand() % 2;
    if (disappear && board->roads[i].car.symbol != 'F') {
        initCar(board, i);
        return true;
    }
    return false;
}
// Update the position of the cars
void updateCars(Board* board, Object* frog) {
    for (int i = 0; i < board->num_roads; i++) {
        Car* car = &board->roads[i].car;
        clock_t current_time = clock(); // Calulate the last time since last move
        double passed_time = double(current_time - car->last_move_time) / CLOCKS_PER_SEC;
        if (car->symbol == 'S') { // Checking if the car should stop
            if (car->x == frog->x || car->x == frog->x - 1) {

                if (car->direction == 1 && frog->y > car->y && frog->y - car->y <= 2) {
                    car->stop_now = true;
                }
                else if (car->direction == -1 && frog->y < car->y && car->y - frog->y <= 2) {
                    car->stop_now = true;
                }
                else {
                    car->stop_now = false;
                }
            }
            else {
                car->stop_now = false;
            }
        }
        if (!car->stop_now && passed_time >= 1.0 / car->speed) { // Car speed delay
            car->y += car->direction;
            if (frog->friendly_car_index == i && frog->on_friendly_car) {
                frog->y += car->direction;
            }
            if (car->y >= NUMCOLS - 1) {
                if (!disappearCar(board, i)) {
                    if (frog->friendly_car_index == i && frog->on_friendly_car) {
                        frog->y = NUMCOLS - 2;
                        frog->on_friendly_car = false;
                    }
                    car->y = 0;
                }
            }
            if (car->y < 0) {
                if (!disappearCar(board, i)) {
                    if (frog->friendly_car_index == i && frog->on_friendly_car) {
                        frog->y = 0;
                        frog->on_friendly_car = false;
                    }
                    car->y = NUMCOLS - 2;
                }
            }
            car->last_move_time = current_time;
        }
    }
}

// Function to randomly change the speed of the cars during the game
void updateCarsSpeed(Board* board) {
    for (int i = 0; i < board->num_roads; i++) {
        int change_speed = rand() % 2; // 50% chance to change the speed
        if (change_speed) {
            board->roads[i].car.speed = board->car_min_speed + rand() % (board->car_max_speed - board->car_min_speed + 1);
        }
    }
}

// Function to draw the cars
void drawCars(Board* board) {
    for (int i = 0; i < board->num_roads; i++) {
        if (board->roads[i].car.symbol == 'F' && board->friendly_on) {
            attron(COLOR_PAIR(FRIENDLY_COLOR));
            mvaddch(board->roads[i].car.x, board->roads[i].car.y, board->roads[i].car.symbol);
            attroff(COLOR_PAIR(FRIENDLY_COLOR));
        }
        else {
            mvaddch(board->roads[i].car.x, board->roads[i].car.y, board->roads[i].car.symbol);
        }
    }
}

//**************************
//* FROG RELATED FUNCTIONS *
//**************************

// Function to initialize the frog
void initObject(Object* frog, int x, int y, char symbol, int speed) {
    frog->x = x;
    frog->y = y;
    frog->symbol = symbol;
    frog->speed = speed;
    frog->level = 1;
    frog->last_move_time = clock(); // Initialize the time of the last move
    frog->last_key = 0; // Initialize the last clicked key
    frog->lanes_passed = 0;
}


// Function to draw the frog
void drawObject(const Object* frog) {
    attron(COLOR_PAIR(FROG_COLOR));
    mvaddch(frog->x, frog->y, frog->symbol);
    attroff(COLOR_PAIR(FROG_COLOR));
}

// Function to move the frog up
void moveUp(Object* frog, const Board* board, clock_t current_time) {
    if (frog->x > 0) {
        bool noObstacle = true;
        if (board->free_rows[frog->x - 1].is_free == true) { // checking if there is no obstacle in the row above
            if (board->free_rows[frog->x - 1].obstacles[frog->y] == true) {
                noObstacle = false;
            }
        }
        if (noObstacle) {
            frog->x--;
            frog->lanes_passed++;
            frog->last_key = 0;
            frog->last_move_time = current_time;
        }
    }
}

// Function to move the frog down
void moveDown(Object* frog, const Board* board, clock_t current_time) {
    if (frog->x < board->rows - 1) {
        bool noObstacle = true;
        if (board->free_rows[frog->x + 1].is_free == true) { // checking if there is no obstacle in the row below
            if (board->free_rows[frog->x + 1].obstacles[frog->y] == true) {
                noObstacle = false;
            }
        }
        if (noObstacle) {
            frog->x++;
            frog->lanes_passed--;
            frog->last_key = 0;
            frog->last_move_time = current_time;
        }
    }
}

// Function to move the frog left
void moveLeft(Object* frog, const Board* board, clock_t current_time) {
    if (frog->y > 0) {
        bool noObstacle = true;
        if (board->free_rows[frog->x].is_free == true) { // checking if there is no obstacle to the left
            if (board->free_rows[frog->x].obstacles[frog->y - 1] == true) {
                noObstacle = false;
            }
        }
        if (noObstacle) {
            frog->y--;
            frog->last_key = 0;
            frog->last_move_time = current_time;
        }
    }
}

// Function to move the frog right
void moveRight(Object* frog, const Board* board, clock_t current_time) {
    if (frog->y < board->cols - 2) {
        bool noObstacle = true;
        if (board->free_rows[frog->x].is_free == true) { // checking if there is no obstacle to the right
            if (board->free_rows[frog->x].obstacles[frog->y + 1] == true) {
                noObstacle = false;
            }
        }
        if (noObstacle) {
            frog->y++;
            frog->last_key = 0;
            frog->last_move_time = current_time;
        }
    }
}


// Function to move the frog
void moveObject(Object* frog, int ch, const Board* board) {
    clock_t current_time = clock(); // Calulate the last time since last move
    double passed_time = double(current_time - frog->last_move_time) / CLOCKS_PER_SEC;

    if (ch == KEY_UP || ch == KEY_DOWN || ch == KEY_LEFT || ch == KEY_RIGHT) {
        frog->last_key = ch; // Save the last key pressed
        frog->on_friendly_car = false;
    }

    if (passed_time >= 1.0 / frog->speed) { // frog speed delay
        if (frog->last_key == KEY_UP) {
            moveUp(frog, board, current_time);
        }
        else if (frog->last_key == KEY_DOWN) {
            moveDown(frog, board, current_time);
        }
        else if (frog->last_key == KEY_LEFT) {
            moveLeft(frog, board, current_time);
        }
        else if (frog->last_key == KEY_RIGHT) {
            moveRight(frog, board, current_time);
        }
    }
}

//***************************
//* STORK RELATED FUNCTIONS *
//***************************

void initStork(Stork* stork, Object* frog) {
    stork->x = NUMROWS - 1;
    stork->y = 0;
    stork->symbol = 'B';
    stork->speed = frog->speed / 2;
    stork->last_move_time = clock();
}

void updateStork(Stork* stork, Object* frog) {
    clock_t current_time = clock();
    double passed_time = double(current_time - stork->last_move_time) / CLOCKS_PER_SEC;
    if (passed_time >= 1.0 / stork->speed) {
        if (stork->y < frog->y) {
            stork->y++;
        }
        else if (stork->y > frog->y) {
            stork->y--;
        }
        if (stork->x > frog->x) {
            stork->x--;
        }
        else if (stork->x < frog->x) {
            stork->x++;
        }
        stork->last_move_time = current_time;
    }
}

void drawStork(Stork* stork) {
    attron(COLOR_PAIR(STORK_COLOR));
    mvaddch(stork->x, stork->y, stork->symbol);
    attroff(COLOR_PAIR(STORK_COLOR));
}

//*********************************
//* GAME CURRENT STATUS FUNCTIONS *
//*********************************

// Function to check if the frog collided with a car or a stork
bool checkCollision(Object* frog, Board* board, Stork* stork) {
    //checking a collision with the car
    for (int i = 0; i < board->num_roads; i++) {
        if (frog->x == board->roads[i].car.x && frog->y == board->roads[i].car.y) {
            if (board->roads[i].car.symbol == 'F' && board->friendly_on) {
                frog->on_friendly_car = true;
                frog->friendly_car_index = i;
                return false;
            }
            else {
                return true;
            }
        }
    }
    //checking a collsion with the stork
    if (frog->x == stork->x && frog->y == stork->y) {
        return true;
    }
    return false;
}

// Function to show the number of lanes passed
void showLanesPassed(int x) {
    mvprintw(NUMROWS / 2 + 1, NUMCOLS + 1, "Lanes passed: %d", x);
}

// Function to check if the frog reached the end
bool checkWin(const Object* frog) {
    return frog->x == 0;
}

//************************
//* GAME SCORE FUNCTIONS *
//************************

// Saving the calculated points to the file
void saveScore(int points) {
    FILE* file = fopen("ranking.txt", "a");
    if (file == NULL) {
        printf("Error: ranking.txt file not found!\n");
        return;
    }
    fprintf(file, "%d\n", points);
    fclose(file);
}

// Function that calculate points after the end of the game
void CalculatePoints(Board* board, Object* frog, Timer* timer) {
    int lanes_passed = frog->lanes_passed;
    double time_passed = timer->current_time;
    int win = checkWin(frog);
    int bonus_for_win = (1 + win);
    int minus_for_speed = 30 * frog->speed;
    int points = 0;
    points += (lanes_passed * 60 - 3 * time_passed) * bonus_for_win;
    points -= minus_for_speed;
    if (points < 0) {
        points = 0;
    }
    saveScore(points);
}

// Deleting all the scores for the file to be clear for the next game
void clearScoreFile() {
    FILE* file = fopen("ranking.txt", "w");
    if (file == NULL) {
        printf("Error: ranking.txt file not found!\n");
        return;
    }
    fclose(file);
}

//******************
//* MENU FUNCTIONS *
//******************

void showRanking(WINDOW* menu_win) {
    mvwprintw(menu_win, 1, 20, "Ranking");
    FILE* file = fopen("ranking.txt", "r");
    if (file == NULL) {
        mvwprintw(menu_win, 2, 1, "No scores yet!");
        return;
    }
    else {
        int score;
        int i = 2;
        while (fscanf(file, "%d", &score) == 1) {
            mvwprintw(menu_win, i, 20, "%d: %d points", i - 1, score);
            i++;
        }
    }
    fclose(file);
}

void showInstructions(WINDOW* menu_win) {
    mvwprintw(menu_win, 8, 40, "Instructions:");
    mvwprintw(menu_win, 9, 40, "-Use arrow keys to move the frog");
    mvwprintw(menu_win, 10, 40, "-Avoid cars and stork");
    mvwprintw(menu_win, 11, 40, "-Reach the top to the reach the next level");
    mvwprintw(menu_win, 12, 40, "-Complete all three levels to win");
    mvwprintw(menu_win, 13, 40, "-Press space to turn on/off friendly cars");
}

void showInfo(WINDOW* menu_win) {
    mvwprintw(menu_win, 1, 40, "Symbols and their meanings:");

    // Frog symbol
    wattron(menu_win, COLOR_PAIR(FROG_COLOR));
    mvwaddch(menu_win, 2, 40, 'O');
    wattroff(menu_win, COLOR_PAIR(FROG_COLOR));
    mvwprintw(menu_win, 2, 42, "- Frog");

    // Car symbol
    mvwaddch(menu_win, 3, 40, 'C');
    mvwprintw(menu_win, 3, 42, "- Car");

    // Stopping Car symbol
    mvwaddch(menu_win, 3, 40, 'S');
    mvwprintw(menu_win, 3, 42, "- Stopping Car");

    // Friendly car symbols
    mvwaddch(menu_win, 5, 40, 'F');
    mvwprintw(menu_win, 5, 42, "- Friendly Car Off");

    wattron(menu_win, COLOR_PAIR(FRIENDLY_COLOR));
    mvwaddch(menu_win, 6, 40, 'F');
    wattroff(menu_win, COLOR_PAIR(FRIENDLY_COLOR));
    mvwprintw(menu_win, 6, 42, "- Friendly Car On");

    // Stork symbol
    wattron(menu_win, COLOR_PAIR(STORK_COLOR));
    mvwaddch(menu_win, 4, 40, 'B');
    wattroff(menu_win, COLOR_PAIR(STORK_COLOR));
    mvwprintw(menu_win, 4, 42, "- Stork");

    // Obstacle symbol
    wattron(menu_win, COLOR_PAIR(OBSTACLE_COLOR));
    mvwaddch(menu_win, 7, 40, 'X');
    wattroff(menu_win, COLOR_PAIR(OBSTACLE_COLOR));
    mvwprintw(menu_win, 7, 42, "- Obstacle");
}

int showMenu() {
    int ch;
    int highlight = 0;

    WINDOW* menu_win = newwin(15, 85, 0, 0); // Creating a window big enough for the menu
    box(menu_win, 0, 0);
    keypad(menu_win, TRUE);

    showRanking(menu_win);

    showInfo(menu_win);

    showInstructions(menu_win);

    while (true) {
        // Highlight the selected option
        if (highlight == 0) {
            wattron(menu_win, A_REVERSE);
            mvwprintw(menu_win, 1, 1, "Start");
            wattroff(menu_win, A_REVERSE);
            mvwprintw(menu_win, 2, 1, "Exit");
        }
        else if (highlight == 1) {
            wattroff(menu_win, A_REVERSE);
            mvwprintw(menu_win, 1, 1, "Start");
            wattron(menu_win, A_REVERSE);
            mvwprintw(menu_win, 2, 1, "Exit");
        }

        ch = wgetch(menu_win);
        if (ch == KEY_UP) {
            highlight = 0;
        }
        else if (ch == KEY_DOWN) {
            highlight = 1;
        }
        else if (ch == 10) { // If enter key is pressed then quit menu
            delwin(menu_win);
            return highlight;
        }
    }
}

//******************
//* GAME MAIN LOOP *
//******************

void printEverything(Board* board, Object* frog, Stork* stork, Timer* timer) {
    clear();
    printBoard(board);
    drawCars(board);
    drawObject(frog);
    drawStork(stork);
    mvprintw(NUMROWS / 2 - 1, NUMCOLS + 1, "Level: %d ", frog->level);
    showTimer(timer);
    showLanesPassed(frog->lanes_passed);
    mvprintw(NUMROWS / 2 + 2, NUMCOLS + 1, "Frog speed: %d ", frog->speed);
    mvprintw(NUMROWS / 2 + 3, NUMCOLS + 1, "Car min/max speed: %d/%d ", board->car_min_speed, board->car_max_speed);
    mvprintw(NUMROWS / 2 + 4, NUMCOLS + 1, "Jan Rudnicki, 203179");
    mvprintw(LINES - 1, 0, "Press q to exit");
}

void GameLoop(Board* board, Object* frog, Timer* timer, Stork* stork) {
    nodelay(stdscr, TRUE);
    int ch;
    while ((ch = getch()) != 'q') {
        updateTimer(timer);
        updateCars(board, frog);
        moveObject(frog, ch, board);
        updateStork(stork, frog);
        if (int(timer->current_time) % 10 == 0 && int(timer->current_time) != 0) {
            updateCarsSpeed(board);
        }
        if (ch == ' ') {
            FriendlyOnOff(board);
        }
        printEverything(board, frog, stork, timer);
        if (checkCollision(frog, board, stork)) {
            mvprintw(NUMROWS / 2, NUMCOLS + 1, "Game Over! Press any key to return to menu.");
            nodelay(stdscr, FALSE);
            CalculatePoints(board, frog, timer);
            getch();
            break;
        }
        if (checkWin(frog)) {
            if (frog->level == 3) {
                CalculatePoints(board, frog, timer);
                mvprintw(NUMROWS / 2, NUMCOLS + 1, "You Win! Press any key to return to menu.");
                nodelay(stdscr, FALSE);
                getch();
                break;
            }
            else {
                initBoard(board, board->car_min_speed + 3, board->car_max_speed + 3);
                frog->level++;
                frog->x = board->rows - 1;
                frog->y = board->cols / 2;
                stork->x = NUMROWS - 1;
                stork->y = 0;
            }
        }
        refresh();
    }
}

//************************************
//* READ PARAMETERS FROM CONFIG FILE *
//************************************

void openConfigFile(int* FROG_SPEED, int* CAR_MIN_SPEED, int* CAR_MAX_SPEED) {
    FILE* configFile = fopen("config.txt", "r");
    if (configFile == NULL) {
        printf("Error: config.txt file not found!\n");
        return;
    }
    char key[30];
    int value;
    for (int i = 0; i < 3; i++) {
        if (fscanf(configFile, "%29s%d\n", key, &value) == 2) {
            if (i == 0) {
                *FROG_SPEED = value;
            }
            else if (i == 1) {
                *CAR_MIN_SPEED = value;
            }
            else {
                *CAR_MAX_SPEED = value;
            }
        }
    }
    fclose(configFile);
}

//*****************
//* MAIN FUNCTION *
//*****************

int main() {
    initscr();
    clear();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    srand(time(NULL));
    clearScoreFile();
    createColorPairs();

    // Initialize the parameters that will be read from the config file
    int FROG_SPEED;
    int CAR_MIN_SPEED;
    int CAR_MAX_SPEED;

    while (true) {
        // Show menu
        int choice = showMenu();
        if (choice == 1) { // Exit
            break;
        }

        // Read parameters from config file
        openConfigFile(&FROG_SPEED, &CAR_MIN_SPEED, &CAR_MAX_SPEED);

        // Initialize the board
        Board* board = new Board;
        initBoard(board, CAR_MIN_SPEED, CAR_MAX_SPEED);
        printBoard(board);

        // Initialize the frog
        Object* frog = new Object;
        initObject(frog, board->rows - 1, board->cols / 2, 'O', FROG_SPEED);
        drawObject(frog);

        Stork* stork = new Stork;
        initStork(stork, frog);

        // Initialize the timer
        Timer* timer = new Timer;
        initTimer(timer);

        // Start the game loop
        GameLoop(board, frog, timer, stork);

        // Free memory and refresh the screen
        freeMemory(board, frog, timer, stork);
        clear();
        refresh();
    }

    clearScoreFile();
    endwin();
    return 0;
}
