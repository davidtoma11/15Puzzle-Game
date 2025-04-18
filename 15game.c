/*
 * 15-Puzzle Game Implementation using X11/Xlib
 *
 * This program creates a classic sliding puzzle game where players arrange
 * numbered tiles in order by sliding them into the empty space.
 */

#include <X11/Xlib.h>          // X11 graphics library
#include <X11/Xutil.h>         // X11 utilities
#include <X11/Xft/Xft.h>       // X11 font rendering
#include <stdlib.h>            // Standard library functions
#include <stdio.h>             // Standard I/O functions
#include <string.h>            // String manipulation functions
#include <time.h>              // Time functions for timer
#include <unistd.h>            // For usleep()

// Game constants
#define TILE_SIZE 80           // Size of each tile in pixels
#define BOARD_SIZE 4           // Board dimensions (4x4)
#define BORDER 4               // Border between tiles
#define WINDOW_PADDING 20      // Padding around game board
#define HEADER_HEIGHT 60       // Height of the header area

// Tile structure representing each puzzle piece
typedef struct {
    int value;  // Tile number (0 represents empty space)
    int x, y;   // Board coordinates
} Tile;

// Game state variables
Tile board[BOARD_SIZE][BOARD_SIZE];  // Game board
Display *display;                    // X11 display connection
Window window;                       // Main application window
GC gc;                               // Graphics context
XftDraw *xftdraw;                    // Font drawing context
XftColor black, white, blue, dark_gray;  // Color definitions
int screen;                          // Default screen
int dark_mode = 0;                   // 0 = light mode, 1 = dark mode
int move_count = 0;                  // Count of player moves
time_t start_time;                   // Game start time
time_t last_time_update = 0;         // Last timer update time

/*
 * Initialize the game board with tiles in solved position
 * (Numbers 1-15 with empty space in bottom-right corner)
 */
void initialize_board() {
    int counter = 1;
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            board[y][x].value = (x == BOARD_SIZE-1 && y == BOARD_SIZE-1) ? 0 : counter++;
            board[y][x].x = x;
            board[y][x].y = y;
        }
    }
}

/*
 * Draw game status information (timer and move count)
 */
void draw_status_info() {
    // Calculate elapsed time
    time_t now = time(NULL);
    int seconds = (int)difftime(now, start_time);

    // Format time string
    char timer_text[32];
    snprintf(timer_text, sizeof(timer_text), "Time: %02d:%02d", seconds / 60, seconds % 60);

    // Format move count string
    char move_text[32];
    snprintf(move_text, sizeof(move_text), "Moves: %d", move_count);

    // Load font and draw status info
    XftFont *info_font = XftFontOpenName(display, screen, "Arial-9");
    if (info_font) {
        // Draw timer
        XftDrawStringUtf8(xftdraw, dark_mode ? &white : &black, info_font,
                          WINDOW_PADDING + 3 , 30,
                          (FcChar8*)timer_text, strlen(timer_text));

        // Draw move count
        XftDrawStringUtf8(xftdraw, dark_mode ? &white : &black, info_font,
                          WINDOW_PADDING +3 , 50,
                          (FcChar8*)move_text, strlen(move_text));
    }
}

/*
 * Draw the game header with title, status info, and buttons
 */
void draw_header() {
    // Draw header background
    XSetForeground(display, gc, dark_mode ? dark_gray.pixel : WhitePixel(display, screen));
    XFillRectangle(display, window, gc, 0, 0,
                  BOARD_SIZE*(TILE_SIZE+BORDER)+WINDOW_PADDING*2,
                  HEADER_HEIGHT);

    // Draw game title
    XftFont *title_font = XftFontOpenName(display, screen, "Ubuntu-20");
    if (title_font) {
        const char *title = "Game 15";
        XGlyphInfo extents;
        XftTextExtents8(display, title_font, (FcChar8*)title, strlen(title), &extents);
        int title_width = extents.xOff;
        int win_width = BOARD_SIZE*(TILE_SIZE+BORDER) + WINDOW_PADDING*2;

        // Center the title
        int text_x = (win_width - title_width) / 2;
        int text_y = HEADER_HEIGHT / 2 + (title_font->ascent - title_font->descent) / 2 + 10;

        XftDrawStringUtf8(xftdraw, dark_mode ? &white : &black, title_font,
                          text_x, text_y,
                          (FcChar8*)title, strlen(title));
    }

    // Draw status info (timer and moves)
    draw_status_info();

    int win_width = BOARD_SIZE*(TILE_SIZE+BORDER) + WINDOW_PADDING*2;

    // Button dimensions and positions
    int button_width = 90;
    int button_height = 22;
    int button_x = win_width - WINDOW_PADDING - button_width;
    int reset_y = 10;
    int toggle_y = reset_y + button_height + 8;  // 8px spacing between buttons

    // Draw Reset button
    XSetForeground(display, gc, blue.pixel);
    XFillRectangle(display, window, gc, button_x, reset_y, button_width, button_height);

    XftFont *btn_font = XftFontOpenName(display, screen, "Arial-9");
    if (btn_font) {
        XftDrawStringUtf8(xftdraw, &white, btn_font,
                          button_x + 20, reset_y + 15,
                          (FcChar8*)"RESET", 5);
    }

    // Draw Dark/Light Mode toggle button
    XSetForeground(display, gc, blue.pixel);
    XFillRectangle(display, window, gc, button_x, toggle_y, button_width, button_height);

    const char *mode_label = dark_mode ? "LIGHT MODE" : "DARK MODE";
    XftDrawStringUtf8(xftdraw, &white, btn_font,
                      button_x + 10, toggle_y + 15,
                      (FcChar8*)mode_label, strlen(mode_label));
}

/*
 * Draw the game board with all tiles
 */
void draw_board() {
    // Draw board background
    XSetForeground(display, gc, dark_mode ? dark_gray.pixel : WhitePixel(display, screen));
    XFillRectangle(display, window, gc, 0, HEADER_HEIGHT,
                  BOARD_SIZE*(TILE_SIZE+BORDER)+WINDOW_PADDING*2,
                  BOARD_SIZE*(TILE_SIZE+BORDER)+WINDOW_PADDING*2);

    // Draw each tile
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            int pos_x = WINDOW_PADDING + x*(TILE_SIZE+BORDER);
            int pos_y = HEADER_HEIGHT + WINDOW_PADDING + y*(TILE_SIZE+BORDER);

            if (board[y][x].value != 0) {
                // Draw tile background
                if (dark_mode) {
                    XSetForeground(display, gc, white.pixel);  // White tile in dark mode
                } else {
                    XSetForeground(display, gc, BlackPixel(display, screen));  // Black tile in light mode
                }
                XFillRectangle(display, window, gc, pos_x, pos_y, TILE_SIZE, TILE_SIZE);

                // Prepare tile number text
                char num[3];
                snprintf(num, sizeof(num), "%d", board[y][x].value);

                XftFont *font = XftFontOpenName(display, screen, "Arial-22");
                if (font) {
                    XGlyphInfo extents;
                    XftTextExtentsUtf8(display, font, (FcChar8*)num, strlen(num), &extents);

                    // Center text in tile
                    int text_x = pos_x + (TILE_SIZE - extents.width) / 2;
                    int text_y = pos_y + (TILE_SIZE + font->ascent - font->descent) / 2;

                    // Draw number (black in dark mode, white in light mode)
                    XftDrawStringUtf8(xftdraw, dark_mode ? &black : &white, font,
                                      text_x, text_y,
                                      (FcChar8*)num, strlen(num));
                }
            }
        }
    }
}

/*
 * Check if the current board configuration is solvable
 * (Following 15-puzzle solvability rules)
 */
int is_solvable() {
    int inversions = 0;
    int flat_board[BOARD_SIZE * BOARD_SIZE - 1];  // Ignore empty space
    int k = 0;

    // Flatten board to 1D array (excluding empty space)
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            if (board[y][x].value != 0) {
                flat_board[k++] = board[y][x].value;
            }
        }
    }

    // Count inversions (tiles out of order)
    for (int i = 0; i < k; i++) {
        for (int j = i + 1; j < k; j++) {
            if (flat_board[i] > flat_board[j]) {
                inversions++;
            }
        }
    }

    // Find row number of empty space (0-indexed from top)
    int empty_row;
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            if (board[y][x].value == 0) {
                empty_row = y;
                break;
            }
        }
    }

    // For 4x4 puzzle: (inversions + empty_row) must be odd
    return (inversions + empty_row) % 2 == 1;
}

/*
 * Shuffle the board to start a new game
 * Makes random valid moves to ensure solvable configuration
 */
void shuffle_board() {
    srand(time(NULL));
    int empty_x = BOARD_SIZE-1, empty_y = BOARD_SIZE-1;

    // Make random valid moves (like a player would)
    for (int i = 0; i < 200 + rand() % 100; i++) {
        int dir = rand() % 4;
        int new_x = empty_x, new_y = empty_y;

        // Calculate new position based on random direction
        switch (dir) {
            case 0: if (empty_y > 0) new_y--; break;          // Up
            case 1: if (empty_y < BOARD_SIZE-1) new_y++; break; // Down
            case 2: if (empty_x > 0) new_x--; break;          // Left
            case 3: if (empty_x < BOARD_SIZE-1) new_x++; break; // Right
        }

        // If move is valid, swap with empty space
        if (new_x != empty_x || new_y != empty_y) {
            board[empty_y][empty_x].value = board[new_y][new_x].value;
            board[new_y][new_x].value = 0;
            empty_x = new_x;
            empty_y = new_y;
        }
    }

    // Ensure puzzle is solvable (if not, swap two non-empty tiles)
    if (!is_solvable()) {
        if (board[0][0].value != 0 && board[0][1].value != 0) {
            int temp = board[0][0].value;
            board[0][0].value = board[0][1].value;
            board[0][1].value = temp;
        }
    }
}

/*
 * Toggle between dark and light color modes
 */
void toggle_dark_mode() {
    dark_mode = !dark_mode;
    draw_header();
    draw_board();
    XFlush(display);
}

/*
 * Reset the game to initial state
 */
void reset_game() {
    initialize_board();
    shuffle_board();
    move_count = 0;
    start_time = time(NULL);
    draw_header();
    draw_board();
    XFlush(display);
}

/*
 * Attempt to move a tile to the empty space
 * Returns 1 if move was valid, 0 otherwise
 */
int move_tile(int x, int y) {
    // Check if position is valid
    if (x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE) {
        return 0; // Invalid move
    }

    // Find empty space position
    int empty_x = -1, empty_y = -1;
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (board[i][j].value == 0) {
                empty_x = j;
                empty_y = i;
                break;
            }
        }
    }

    // Check if selected tile is adjacent to empty space
    if ((abs(x - empty_x) == 1 && y == empty_y) ||
        (abs(y - empty_y) == 1 && x == empty_x)) {
        // Swap tile with empty space
        board[empty_y][empty_x].value = board[y][x].value;
        board[y][x].value = 0;
        move_count++; // Increment move counter
        return 1; // Valid move
    }

    return 0; // Invalid move
}

/*
 * Check if the puzzle is solved (all tiles in order)
 */
int check_win() {
    int counter = 1;
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            // Last position should be empty (0)
            if (y == BOARD_SIZE-1 && x == BOARD_SIZE-1) {
                if (board[y][x].value != 0) return 0;
            } else {
                if (board[y][x].value != counter++) return 0;
            }
        }
    }
    return 1;
}

/*
 * Display win message and reset game after delay
 */
void show_win_message() {
    XftFont *font = XftFontOpenName(display, screen, "Arial-20");
    if (font) {
        const char *msg = "You Win!";
        XGlyphInfo extents;
        XftTextExtents8(display, font, (FcChar8*)msg, strlen(msg), &extents);

        // Center message on screen
        int x = (BOARD_SIZE*(TILE_SIZE+BORDER) - extents.width)/2;
        int y = (HEADER_HEIGHT + BOARD_SIZE*(TILE_SIZE+BORDER))/2;

        // Draw semi-transparent background
        XSetForeground(display, gc, 0x333333);
        XFillRectangle(display, window, gc,
                      x-10, y-font->ascent-10,
                      extents.width+20, font->ascent+font->descent+20);

        // Draw win message
        XftDrawStringUtf8(xftdraw, &white, font, x, y, (FcChar8*)msg, strlen(msg));
        XFlush(display);
    }

    // Wait 3 seconds before resetting game
    sleep(3);
    reset_game();
}

/*
 * Main program entry point
 */
int main() {
    // Initialize X11 display connection
    display = XOpenDisplay(NULL);
    screen = DefaultScreen(display);

    // Calculate window dimensions
    int win_width = BOARD_SIZE*(TILE_SIZE+BORDER) + WINDOW_PADDING*2;
    int win_height = HEADER_HEIGHT + BOARD_SIZE*(TILE_SIZE+BORDER) + WINDOW_PADDING*2;

    // Create main window
    window = XCreateSimpleWindow(display, RootWindow(display, screen),
                                0, 0, win_width, win_height,
                                0,
                                WhitePixel(display, screen),
                                WhitePixel(display, screen));

    // Set window properties
    XStoreName(display, window, "Game 15");
    XSelectInput(display, window, ExposureMask | ButtonPressMask);
    XMapWindow(display, window);

    // Create graphics context and font drawer
    gc = XCreateGC(display, window, 0, NULL);
    xftdraw = XftDrawCreate(display, window,
                           DefaultVisual(display, screen),
                           DefaultColormap(display, screen));

    // Allocate colors
    XftColorAllocName(display, DefaultVisual(display, screen),
                     DefaultColormap(display, screen), "black", &black);
    XftColorAllocName(display, DefaultVisual(display, screen),
                     DefaultColormap(display, screen), "white", &white);
    XftColorAllocName(display, DefaultVisual(display, screen),
                     DefaultColormap(display, screen), "#4285F4", &blue); // Modern blue
    XftColorAllocName(display, DefaultVisual(display, screen),
                     DefaultColormap(display, screen), "#2D2D2D", &dark_gray); // Dark gray

    // Initialize game state
    initialize_board();
    shuffle_board();
    start_time = time(NULL);

    // Initial drawing
    draw_header();
    draw_board();
    XFlush(display);

    // Main event loop
    XEvent event;
    while (1) {
        // Check for pending events
        if (XPending(display)) {
            XNextEvent(display, &event);

            if (event.type == ButtonPress) {
                int x = event.xbutton.x;
                int y = event.xbutton.y;

                // Check if click is in header area (buttons)
                if (y < HEADER_HEIGHT) {
                    int win_width = BOARD_SIZE*(TILE_SIZE+BORDER) + WINDOW_PADDING*2;
                    int button_width = 90;
                    int button_height = 22;
                    int button_x = win_width - WINDOW_PADDING - button_width;
                    int reset_y = 10;
                    int toggle_y = reset_y + button_height + 8;

                    // Handle button clicks
                    if (x >= button_x && x <= button_x + button_width) {
                        if (y >= reset_y && y <= reset_y + button_height) {
                            reset_game();
                        } else if (y >= toggle_y && y <= toggle_y + button_height) {
                            toggle_dark_mode();
                        }
                    }
                }
                // Click on game board
                else {
                    // Calculate which tile was clicked
                    int tile_x = (x - WINDOW_PADDING) / (TILE_SIZE + BORDER);
                    int tile_y = (y - HEADER_HEIGHT - WINDOW_PADDING) / (TILE_SIZE + BORDER);

                    // Validate coordinates
                    if (tile_x >= 0 && tile_x < BOARD_SIZE &&
                        tile_y >= 0 && tile_y < BOARD_SIZE) {

                        // Attempt to move tile
                        if (move_tile(tile_x, tile_y)) {
                            draw_board();
                            XFlush(display);

                            // Check for win condition
                            if (check_win()) {
                                show_win_message();
                            }
                        }
                    }
                }
            }
        }

        // Update timer every second
        time_t now = time(NULL);
        if (now != last_time_update) {
            last_time_update = now;
            draw_header();
            XFlush(display);
        }

        // Small delay to reduce CPU usage
        usleep(10000); // 10ms
    }

    // Cleanup resources (unreachable in current implementation)
    XftDrawDestroy(xftdraw);
    XFreeGC(display, gc);
    XDestroyWindow(display, window);
    XCloseDisplay(display);
    return 0;
}
