/****************************************************************************************************
 * Current goal: Fix segfault that occurs when adding new row/column during movement.               *
 *                                                                                                  *
 *                                                                                                  *
 *                                                                                                  *
 *                                                                                                  *
 *                                                                                                  *
 *                                                                                                  *
 *                                                                                                  *
 *                                                                                                  *
 *                                                                                                  *
 *                                                                                                  *
 *                                                                                                  *
 *                                                                                                  *
 *                                                                                                  *
 *                                                                                                  *
 ****************************************************************************************************/













/****************************************************************************************************
 * Name: static_maze_maker.c                                                                        *
 * File creation date: 2023-11-1                                                                    *
 * 1.0 date:                                                                                        *
 * Last modification date:                                                                          *
 * Author: Ryan Wells                                                                               *
 * Additional credit: Thanks to Josh Taylor (joshtaylor314.dev | github.com/babybluesedans)         *
 *                      for the initial drafts of calculate_letter_digits() and lower_boundary()    *
 * Purpose: Tool for creating hand-made mazes for use in IF maze exploration program                *
 *          (preventing the need for coding each maze individually).                                *
 ****************************************************************************************************/

// /* OS-Dependent Conditional Compilation Section */
// //UNIX (only):
// #if !defined(_WIN32) && (defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)))
// #define UNIX
// #include <termios.h>
// #include <unistd.h>
// #define STDIN_FD 0 //0 is the file descriptor for standard input (1 is for std output, 2 is for std err)
// #else
// #undef UNIX
// #endif

// //Windows (only):
// #if defined(_WIN32) && !(defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)))
// #define WINDOWS
// #include <conio.h>
// #else
// #undef WINDOWS
// #endif

// //Neither:
// #if !defined(_WIN32) && !(defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)))
// #define FORCE_BUFFERED_MODE
// #else
// #undef FORCE_BUFFERED_MODE
// #endif

// //Both!?:
// #if defined(_WIN32) && (defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)))
// #define FORCE_BUFFERED_MODE
// #else
// #undef FORCE_BUFFERED_MODE
// #endif

//Testing:
#define FORCE_BUFFERED_MODE


/* Preprocessing Directives (#include) */
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdint.h>

/* Preprocessing Directives (#define) */
#define CLEAR_CONSOLE (void) printf("\033[H\033[2J\033[3J") // ANSI escapes for clearing screen and scrollback.
#define NUM_CARDINAL_DIRECTIONS 4
#define MAX_DISPLAY_HEIGHT 20
#define MAX_DISPLAY_WIDTH 20
#define MAX_COORDINATE 321272405 // Because of the use of the pow() function, combined with the int32_t limit.
#define NUM_LETTERS 26
#define MAX_ID ((MAX_COORDINATE + 1) * (MAX_COORDINATE + 1))
#define ALPHABET "ABCDEFGHIJKLMNOPQRSTUVWXYZ"

/* Type Definitions */
enum cardinal_directions
{
    NORTH,
    EAST,
    SOUTH,
    WEST,
};

typedef struct room
{
    long long id;
    char *id_alias;
    int32_t y_coordinate;
    int32_t x_coordinate;
    bool exists;
    bool exits[4];
    struct room *next_room;
} Room;

typedef struct dimensions
{
    int32_t height;
    int32_t width;
} Dimensions;

typedef struct map
{
    int32_t height;
    int32_t width;
    Room *root; // Pointer to start of linked list containing all rooms
} Map;

typedef struct display
{
    Room ***layout;
    int height;
    int width;
    int32_t y_offset;
    int32_t x_offset;
} Display;

typedef struct command_c
{
    int c;
    struct command_c *next_c;
} Command_C;

typedef struct gamestate
{
    bool quit;
    Display *display;
    Room *current_cursor_focus;
    Map *current_map;
} Gamestate;

/* Declarations of External Variables */
// none

/* Declarations of Global Variables */
int error_code = 0;

/* Prototypes for non-main functions */
void gobble_line(void);
Dimensions prompt_for_dimensions(void);
Map *create_map(Dimensions dim);
Room *make_room(int32_t y_coordinate, int32_t x_coordinate, long long room_id);
Map *load_map(void);
Gamestate *load_gamestate(void);
Map *edit_map(Map *editable_map, Gamestate *current_gamestate);
Room ***create_initial_layout(Map *map_to_display);
void free_layout(Room ***layout, int32_t height);
Display *initialize_display(Room ***layout_array, int32_t array_height, int32_t array_width, Room *root);
Gamestate *initialize_gamestate(Display *display, Map *current_map);
void print_display(Gamestate *g);
char *ystr(int32_t y_coordinate);
int calculate_letter_digits(int32_t number_to_convert);
int32_t lower_boundary(int base, int power);
int calculate_letter_index(int32_t current_number, int current_digit, int32_t lower_boundary);
int get_command(char *prompt);
void free_command(Command_C *root);
int parse_command(char *command);
int caseless_strcmp(char *str1, char *str2);
void obey_command(int command_code, Gamestate *g);
void print_command_listing(void);
void move_cursor(int cardinal_direction, Gamestate *g);
void add_row_north(Gamestate *g);
void add_column_east(Gamestate *g);
void add_row_south(Gamestate *g);
void add_column_west(Gamestate *g);
void save_map(Map *savable_map);
void free_map(Map *freeable_map);
void free_rooms(Room *r);

/* Definition of main */
/*****************************************************************************************
 * main:                Purpose: Runs main menu loop.                                    *
 *                      Parameters: none                                                 *
 *                      Return value: int                                                *
 *                      Side effects: - Clears screen and scrollback                     *
 *                                    - Prints to stdout                                 *
 *                                    - Reads from stdin                                 *
 *                                    - Terminates program                               *
 *****************************************************************************************/
int main(void)
{
    // Main menu:
    // Option: Create new map to edit
    // Option: Load existing map to edit
    // Option: Quit program

    // Main menu loop:
    for (;;)
    {
        // Display main menu:
        CLEAR_CONSOLE;
        (void) printf("Main Menu:\n");
        (void) printf("1. Create new map to edit\n");
        (void) printf("2. Load existing map to edit\n");
        (void) printf("3. Quit program\n\n");

        // Prompt for and validate user input:
        int selection = 0;
        for (;;)
        {
            (void) printf("Enter option number:\n>");
            (void) scanf("%d", &selection), gobble_line();

            if (selection < 1 || selection > 3)
                (void) printf("Please pick from the available options.\n");
            else
                break;
        }

        // Parse user input:
        switch (selection)
        {
            case 1: free_map(edit_map(create_map(prompt_for_dimensions()), NULL)); break;
            case 2: free_map(edit_map(load_map(), load_gamestate())); break;
            default: goto quit;
        }
        if (error_code) break;
    }
    // Exit program:
    quit:
    switch (error_code)
    {
        default: break;
        case 1: (void) printf("Encountered unexpected error. Error code 1: Unable to find room with matching coordinates.\n"); break;
        case 2: (void) printf("Encountered error. Error code 2: Unable to allocate memory for the layout's worth of rows.\n"); break;
        case 3: (void) printf("Encountered error. Error code 3: Unable to allocate memory for the columns in one or all of the layout's rows.\n"); break;
        case 4: (void) printf("Encountered error. Error code 4: Unable to allocate memory for map.\n"); break;
        case 5: (void) printf("Encountered error. Error code 5: Unable to allocate memory for one or more of the map's rooms.\n"); break;
        case 6: (void) printf("Encountered error. Error code 6: Unable to allocate memory for the display.\n"); break;
        case 7: (void) printf("Encountered error. Error code 7: Unable to allocate memory for x_str.\n"); break;
        case 8: (void) printf("Encountered error. Error code 8: Unable to allcoate memory for ystr.\n"); break;
        case 9: (void) printf("Encountered error. Error code 9: Unable to allocate memory for one or more characters in the command string.\n"); break;
        case 10: (void) printf("Encountered error. Error code 10: Unable to allocate memory for stringified linked list of command characters.\n"); break;
        case 11: (void) printf("Encountered unexpected error. Error code 11: Received unknown command_code; cannot obey.\n"); break;
        case 12: (void) printf("Encountered unexpected error. Error code 12: Cannot move cursor in unknown direction.\n"); break;
        case 13: (void) printf("Encountered error. Error code 13: Unable to allocate memory for gamestate.\n"); break;
    }
    return error_code;
}

/* Definitions of other functions */
/*****************************************************************************************
 * gobble_line():    Purpose: Reads & discards whatever is in stdin                      *
 *                   Parameters: none                                                    *
 *                   Return value: none                                                  *
 *                   Side effects: - Reads from stdin                                    *
 *****************************************************************************************/
void gobble_line(void)
{
    char c = 0;
    while ((c = getchar()) != '\n' && c != EOF);
    return;
}

Dimensions prompt_for_dimensions(void)
{
    CLEAR_CONSOLE;
    (void) printf("Creating blank map...\n");

    Dimensions dim;
    dim.height = 0;
    dim.width = 0;

    // Prompt for initial height:
    for (;;)
    {
        (void) printf("Enter desired initial height of map: ");
        (void) scanf("%d", &(dim.height)), gobble_line();
        if (dim.height < 1)
            (void) printf("Please enter an integer greater than zero.\n");
        else if (dim.height > MAX_DISPLAY_HEIGHT)
            (void) printf("Max initial height is %d. More height can be added during editing.\n", MAX_DISPLAY_HEIGHT);
        else
            break;
    }

    // Prompt for initial width:
    for (;;)
    {
        (void) printf("Enter desired initial width of map: ");
        (void) scanf("%d", &(dim.width)), gobble_line();
        if (dim.width < 1)
            (void) printf("Please enter an integer greater than zero.\n");
        else if (dim.width > MAX_DISPLAY_WIDTH)
            (void) printf("Max initial width is %d. More width can be added during editing.\n", MAX_DISPLAY_WIDTH);
        else
            break;
    }

    return dim;
}

/*****************************************************************************************
 * create_map:    Purpose: Creates blank map for further editing.                        *
 *                Parameters: none                                                       *
 *                Return value: Map * -> The created map, to be passed into editing.     *
 *                Side effects: - Clears screen and scrollback                           *
 *                              - Prints to stdout.                                      *
 *                              - Reads from stdin.                                      *
 *                              - Allocates memory.                                      *
 *                              - Edits global variable "error_code"                     *
 *****************************************************************************************/
Map *create_map(Dimensions dim)
{
    // Allocate memory for map:
    Map *created_map = malloc(sizeof(Map));
    if (created_map == NULL)
    {
        error_code = 4;
        return NULL;
    }

    created_map->height = dim.height;
    created_map->width = dim.width;

    // Initialize linked list of rooms, starting from (0,0):
    created_map->root = NULL;
    long long next_id = 0;
    for (int32_t y = 0; y < created_map->height; y++)
    {
        for (int32_t x = 0; x < created_map->width; x++)
        {
            Room *r = make_room(y, x, next_id++);
            if (error_code) return created_map;
            if (created_map->root == NULL)
            {
                created_map->root = r;
            }
            else
            {
                Room *attach_point = created_map->root;
                while (attach_point->next_room != NULL)
                {
                    attach_point = attach_point->next_room;
                }
                attach_point->next_room = r;
            }
        }
    }
    return created_map;
}

/*********************************************************************************************
 * make_room:    Purpose: Allocates memory for and initializes single room.                  *
 *               Parameters: int32_t y_coordinate -> the y-coordinate to assign to the room  *
 *                           int32_t x_coordinate -> the x-coordinate to assign to the room  *
 *                           long long room_id -> the id to be assigned to the room          *
 *               Return value: Room * -> a pointer to the new room                           *
 *               Side effects: - Allocates memory.                                           *
 *                             - Edits global variable "error_code".                         *
 *********************************************************************************************/
Room *make_room(int32_t y_coordinate, int32_t x_coordinate, long long room_id)
{
    Room *r = malloc(sizeof(Room));
    if (r == NULL)
    {
        error_code = 5;
        return NULL;
    }

    r->y_coordinate = y_coordinate, r->x_coordinate = x_coordinate;
    r->exists = true, r->id = room_id, r->next_room = NULL;
    for (int cardinal_direction = NORTH; cardinal_direction < NUM_CARDINAL_DIRECTIONS; cardinal_direction++)
    {
        r->exits[cardinal_direction] = 0;
    }

    r->id_alias = NULL;

    return r;
}

/*****************************************************************************************
 * load_map:    Purpose: Loads map from file for further editing.                        *
 *              Parameters: none                                                         *
 *              Return value: Map * -> The loaded map, to be passed into editing.        *
 *              Side effects: - Reads from external files.                               *
 *                            - Prints to stdout.                                        *
 *                            - Reads from stdin.                                        *
 *                            - Allocates memory.                                        *
 *****************************************************************************************/
Map *load_map(void)
{
    //TODO
    //  ...as part of the function, perform validation on the map file (for example, that it hasn't been edited so as to expand past MAX_COORDINATE, etc)
    Map *loaded_map = NULL;
    return loaded_map;
}

Gamestate *load_gamestate(void)
{
    //TODO
    // as part of the function, perform validation on the gameplay file (if that makes sense when the time comes)
    Gamestate *loaded_gamestate = NULL;
    return loaded_gamestate;
}

/********************************************************************************************
 * edit_map:    Purpose: The bulk of the program. Allows the user to edit the passed map.   *
 *              Parameters: Map *editable_map -> the map to be edited.                      *
 *              Return value: Map * -> the now-edited map,                                  *
 *                                     to be freed before further program operation         *
 *                                     or program termination.                              *
 *              Side effects: - Clears screen and scrollback                                *
 *                            - Calls save_map, which edits external files.                 *
 *                            - Prints to stdout.                                           *
 *                            - Reads from stdin.                                           *
 *                            - Modifies any and all data associated with passed map.       *
 ********************************************************************************************/
Map *edit_map(Map *editable_map, Gamestate *current_gamestate)
{
    if (error_code) return editable_map;

    Room ***layout;
    Display *display;
    Gamestate *gamestate;

    //Initialize the above three declared variables:
    if (current_gamestate == NULL) // If starting a new map, not loading one:
    {
        // Create layout, display, and gamestate:
        layout = create_initial_layout(editable_map);
        if (error_code)
        {
            free_layout(layout, editable_map->height);
            return editable_map;
        }

        display = initialize_display(layout, editable_map->height, editable_map->width, editable_map->root);
        if (error_code)
        {
            free_layout(layout, editable_map->height);
            return editable_map;
        }

        gamestate = initialize_gamestate(display, editable_map);
        if (error_code)
        {
            free_layout(layout, editable_map->height);
            free(display);
        }
    }
    else // If continuing a loaded file:
    {
        layout = current_gamestate->display->layout;
        display = current_gamestate->display;
        gamestate = current_gamestate;
    }

    // Interaction Loop:
    for (;!gamestate->quit;)
    {
        CLEAR_CONSOLE;

        // Print display:
        print_display(gamestate);
        if (error_code)
        {
            free_layout(layout, editable_map->height);
            free(display);
            free(gamestate);
            return editable_map;
        }

        #ifdef FORCE_BUFFERED_MODE
            obey_command(get_command("Enter command:\n>"), gamestate);
            if (error_code)
            {
                free_layout(layout, editable_map->height);
                free(display);
                free(gamestate);
                return editable_map;
            }
        #endif
        #ifdef UNIX
            //TODO
        #endif
        #ifdef WINDOWS
            //TODO
        #endif


        // TODO: Print command prompt (incl "help for help")
        // TODO: Accept input
        // TODO: Parse user command
        // TODO: Update program state based on command
        // TODO: Update display based on updated state.
        // TODO: Add save/quit functionality.
    }

    // Prompt for cursor start location (represented by *)
    //  Alternately, start cursor on top line, furthest room to left.
    // Allow commands for moving cursor cardinally ("move up")
    // Allow commands for moving cursor cardinally while skipping over non-existent rooms ("skip up")
    // Allow commands for cursor jumping ("jump to A3")
    // Allow commands for establishing connecting doors between rooms (cardinally)
    //      ...for now, force exit matching (eg, adding a north exit to a lower room forces a south exit to be added to the room above).
    //        ...one day, this could be refactored to allow "warping" exits, but it'd probably be better to add that functionality w/ a proper GUI.
    // Allow commands for removing connections
    //      ...force destruction of matching exit (eg, removing a north exit forces removal of south exit from room above)
    // Allow commands for expanding grid by x rows or columns
    //      ...but ensure that this never expands past the MAX_COORDINATE limit.
    //      ...if id for room reaches MAX_ID, renumber all the room ids to erase gaps and shrink the highest id.
    // Allow commands for expanding grid by adding new individual rooms
    //      ...but ensure that this never expands past the MAX_COORDINATE limit.
    //      ...if id for room reaches MAX_ID, renumber all the room ids to erase gaps and shrink the highest id.
    // Allow commands for deleting individual rooms
    //      ...but ensure the row/column count stays above zero
    // Allow commands for retracting grid by subtracting x rows or columns
    //      ...but ensure the row/column count stays above zero
    // Allow commands for establishing entrances and exits to maze (including interior ones)
    // Allow commands for adding id aliases (ids are established programmatically)
    // Allow commands for scrolling view of grid cardinally (in this case, meaning up, down, left, right)
    // Let attempts at moving the cursor past the edges of the grid cause the grid to scroll (if there are rooms in that direction).
    // Allow commands for saving maze to file and reading maze from file
    // Allow commands for translating entire map plus or minus x x-values or y y-values
    // Allow commands to establish critical path (in separate ANSI color)
    // Allow command to highlight which rooms cannot be reached
    // Allow command to determine whether critical path exists


    //TODO: Allow saving map before returning (returning leads to freeing--aka losing--map from memory)
    //TODO: Warns when about to return without saving
    free_layout(layout, editable_map->height);
    free(display);
    free(gamestate);
    return editable_map;
}

/**********************************************************************************************************
 * create_initial_layout:     Purpose: Allocates room for, and initializes,                               *
 *                                     a 2D array containing the ids of rooms to display                  *
 *                                     as visualized map during editing.                                  *
 *                            Parameters: Map *map_to_display -> the map data to create the array from.   *
 *                            Return value: Room *** -> a pointer to the array                        *
 *                            Side effects: - allocates memory                                            *
 *                                          - edits global variable "error_code"                          *
 **********************************************************************************************************/
Room ***create_initial_layout(Map *map_to_display)
{
    //Create 2D grid to store room ids:
    int32_t ncols = map_to_display->width;
    int32_t nrows = map_to_display->height;

    Room ***layout = malloc(sizeof(Room **) * nrows);
    if (layout == NULL)
    {
        error_code = 2;
        return NULL;
    }

    for (int32_t i = 0; i < nrows; i++)
    {
        layout[i] = malloc(sizeof(Room *) * (ncols));
        if (layout[i] == NULL)
        {
            error_code = 3;
            return NULL;
        }
    }

    for (int32_t y = 0; y < nrows; y++)
    {
        for (int32_t x = 0; x < ncols; x++)
        {
            Room *current = map_to_display->root;
            while (current != NULL)
            {
                if (current->y_coordinate == y && current->x_coordinate == x)
                {
                    layout[y][x] = current;
                    break;
                }
                current = current->next_room;
            }
        }
    }

    return layout;
}

/****************************************************************************************************
 * free_layout:    Purpose: Frees all rows in layout, followed by layout.                           *
 *                 Parameters: - Room ***layout -> the layout to be freed                       *
 *                             - int32_t height -> the number of rows in the layout                 *
 *                 Return value: none                                                               *
 *                 Side effects: - Frees all memory associated with given layout. Cannot be undone. *
 ****************************************************************************************************/
void free_layout(Room ***layout, int32_t height)
{
    for (int32_t y = 0; y < height; y++)
    {
        free(layout[y]);
    }

    free(layout);
    return;
}

/****************************************************************************************************************
 * initialize_display:        Purpose: Allocates room for, and initializes, a Display.                          *
 *                            Parameters: Room ***layout_array -> pointer to the 2D layout array to display *
 *                                        int32_t array_height -> the height of the 2D layout array             *
 *                                        int32_t array_width -> the width of the 2D layout array               *
 *                            Return value: Display * -> a pointer to the initialized Display                   *
 *                            Side effects: - allocates memory                                                  *
 *                                          - edits global variable "error_code"                                *
 ****************************************************************************************************************/
Display *initialize_display(Room ***layout_array, int32_t array_height, int32_t array_width, Room *root)
{
    Display *d = malloc(sizeof(Display));
    if (d == NULL)
    {
        error_code = 6;
        return NULL;
    }

    d->layout = layout_array;
    d->height = array_height > MAX_DISPLAY_HEIGHT ? MAX_DISPLAY_HEIGHT : array_height;
    d->width = array_width > MAX_DISPLAY_WIDTH ? MAX_DISPLAY_WIDTH : array_width;
    d->y_offset = 0, d->x_offset = 0;

    return d;
}

/*****************************************************************************************
 * name_of_function:    Purpose:                                                         *
 *                      Parameters (and the meaning of each):                            *
 *                      Return value:                                                    *
 *                      Side effects (such as modifying external variables,              *
 *                          printing to stdout, or exiting the program):                 *
 *****************************************************************************************/
Gamestate *initialize_gamestate(Display *display, Map *current_map)
{
    Gamestate *g = malloc(sizeof(Gamestate));
    if (g == NULL)
    {
        error_code = 13;
        return NULL;
    }

    g->quit = false;
    g->display = display;
    g->current_map = current_map;
    g->current_cursor_focus = display->layout[0][0];

    return g;
}

/*****************************************************************************************
 * print_display:       Purpose: Accepts a given Display and prints it to the screen.    *
 *                      Parameters: Gamestate *g -> pointer to the current gamestate.    *
 *                      Return value: none                                               *
 *                      Side effects: - prints to stdout                                 *
 *                                    - frees memory allocated during printing process   *
 *                                    - edits global variable "error_code"               *
 *****************************************************************************************/
void print_display(Gamestate *g)
{
    // typedef struct display
    // {
    //     Room ***layout;
    //     int height;
    //     int width;
    //     int32_t y_offset;
    //     int32_t x_offset;
    // } Display;

    // Find max screen length of y-coordinates to display, for formatting purposes:
    char *longest_y_string = ystr(g->display->height - 1 + g->display->y_offset);
    if (error_code) return;
    int longest_letter_digits = strlen(longest_y_string);
    free(longest_y_string);

    // Find max screen length of x-coordinates to display, for formatting purposes:
    int longest_number_digits = snprintf(NULL, 0, "%d", g->display->width - 1 + g->display->x_offset);

    //Find printing width of one room + surrounding symbols:
    int min_cell_width = 5;
    int space_on_both_sides = 2;
    int cell_width = min_cell_width > longest_number_digits + space_on_both_sides ? min_cell_width : longest_number_digits + space_on_both_sides;
    int assumed_terminal_width_in_cols = MAX_DISPLAY_WIDTH * min_cell_width;
    if (assumed_terminal_width_in_cols < g->display->width * cell_width)
        g->display->width = assumed_terminal_width_in_cols / cell_width;
    int room_width = 3;
    int hyphens = cell_width - room_width;
    int left_hyphens = hyphens / 2;
    int right_hyphens = hyphens - left_hyphens;

    /* Example: *******************
     *      20   21   22          *
     *      |    |                *
     *AAA -( )--( )--( )-         *
     *      |         |           *
     *AAB  ( )  ( )--( )          *
     *      |                     *
     ******************************/

    // Start printing display:
    // First, print x-coordinates row:
    // Print spaces where letter coordinates would go:
    for (int letter_digit = 0; letter_digit < longest_letter_digits; letter_digit++)
        (void) printf(" ");
    (void) printf(" ");
    // Print x-coordinates:
    for (int x = 0; x < g->display->width; x++)
    {
        // Create x-coordinate string:
        int needed_strlen = snprintf(NULL, 0, "%d", x + g->display->x_offset);
        char *x_str = malloc(sizeof(char) * (needed_strlen + 1));
        if (x_str == NULL)
        {
            error_code = 7;
            return;
        }
        (void) snprintf(x_str, sizeof(x_str), "%d", x + g->display->x_offset);
        // Print centered x-coordinate string:
        if (cell_width == needed_strlen + space_on_both_sides)
            (void) printf(" %s ", x_str), free(x_str);
        else
        {
            int spaces_needed = cell_width - needed_strlen;
            int left_spaces = spaces_needed / 2;
            int right_spaces = spaces_needed - left_spaces;
            for (int space = 0; space < left_spaces; space++)
                (void) printf(" ");
            (void) printf("%s", x_str), free(x_str);
            for (int space = 0; space < right_spaces; space++)
                (void) printf(" ");
        }
    }
    (void) printf("\n");

    for (int y = 0; y < g->display->height; y++)
    {
        // Row above room:
        // Print spaces where letter coordinates would go:
        for (int letter_digit = 0; letter_digit < longest_letter_digits; letter_digit++)
            (void) printf(" ");
        (void) printf(" ");

        // Print visible rooms:
        for (int x = 0; x < g->display->width; x++)
        {
            // Print spaces where left hyphens & left side of room would be on room line:
            for (int hyphen = 0; hyphen < left_hyphens + 1; hyphen++) // + 1 is for the left parenthesis of the room.
                (void) printf(" ");

            // Find pointer to room matching current coordinates:
            Room *current = g->display->layout[y + g->display->y_offset][x + g->display->x_offset];
            if (current == NULL)
            {
                error_code = 1;
                return;
            }

            // Print either passageway or spaces depending on north exit per room
            //      (this code assumes a north exit always corresponds with a south exit above):
            if (current->exists && current->exits[NORTH])
                (void) printf("|");
            else
                (void) printf(" ");
            // Print spaces where right side of room & right hyphens would be on room line:
            for (int hyphen = 0; hyphen < right_hyphens + 1; hyphen++) // + 1 is for the right parenthesis of the room.
                (void) printf(" ");
        }
        (void) printf("\n");

        // Row with room:
        // Print letter coordinates:
        char *str = ystr(y + g->display->y_offset);
        if (error_code) return;
        int letter_digits = strlen(str);
        if (longest_letter_digits > letter_digits)
            for (int letter_digit = 0; letter_digit < longest_letter_digits - letter_digits; letter_digit++)
                (void) printf(" ");
        (void) printf("%s ", str), free(str); // y-coordinates are displayed as letters for user QoL
        // Print visible rooms:
        for (int x = 0; x < g->display->width; x++)
        {
            // Find pointer to room matching current coordinates:
            Room *current = g->display->layout[y + g->display->y_offset][x + g->display->x_offset];
            if (current == NULL)
            {
                error_code = 1;
                return;
            }
            // Print either left hyphens or spaces depending on east exit per room
            //      (this code assumes an east exit always corresponds with a west exit to the left):
            for (int hyphen = 0; hyphen < left_hyphens; hyphen++)
                if (current->exists && current->exits[EAST])
                    (void) printf("-");
                else
                    (void) printf(" ");

            // Print room (if existent), with cursor if that's where the cursor is:
            if (current->exists)
                (void) printf("(");
            else
                (void) printf(" ");
            if (current == g->current_cursor_focus)
                (void) printf("*");
            else
                (void) printf(" ");
            if (current->exists)
                (void) printf(")");
            else
                (void) printf(" ");

            // Print eighter right hyphens or spaces depending on west exit per room
            //      (this code assumes a west exit always corresponds with an east exit to the right):
            for (int hyphen = 0; hyphen < right_hyphens; hyphen++)
                if (current->exists && current->exits[WEST])
                    (void) printf("-");
                else
                    (void) printf(" ");
        }
        (void) printf("\n");

        // If this is last row of rooms, print row beneath room:
        if (y == g->display->height - 1)
        {
            // Print spaces where letter coordinates would go:
            for (int letter_digit = 0; letter_digit < longest_letter_digits; letter_digit++)
                (void) printf(" ");
            (void) printf(" ");
            // Print visible rooms:
            for (int x = 0; x < g->display->width; x++)
            {
                // Print spaces where left hyphens & left side of room would be on room line:
                for (int hyphen = 0; hyphen < left_hyphens + 1; hyphen++) // + 1 is for the left parenthesis of the room.
                    (void) printf(" ");
                // Find pointer to room matching current coordinates:
                Room *current = g->display->layout[y + g->display->y_offset][x + g->display->x_offset];
                if (current == NULL)
                {
                    error_code = 1;
                    return;
                }
                // Print either passageway or spaces depending on north exit per room
                //      (this code assumes a south exit always corresponds with a north exit below):
                if (current->exists && current->exits[SOUTH])
                    (void) printf("|");
                else
                    (void) printf(" ");
                // Print spaces where right side of room & right hyphens would be on room line:
                for (int hyphen = 0; hyphen < right_hyphens + 1; hyphen++) // + 1 is for the right parenthesis of the room.
                    (void) printf(" ");
            }
            (void) printf("\n");
        }
    }

    return;
}

/********************************************************************************************
 * ystr:    Purpose: Converts a given number coordinate into a letter coordinate.           *
 *          Parameters: int32_t y_coordinate -> the number coordinate to be converted       *
 *          Return value: char * -> a pointer to a string containing the letter coordinate  *
 *          Side effects: - allocates memory                                                *
 *                        - edits global variable "error_code"                              *
 ********************************************************************************************/
char *ystr(int32_t y_coordinate)
{
    int letters_wide = calculate_letter_digits(y_coordinate);
    char *str = malloc(sizeof(char) * (letters_wide + 1));
    if (str == NULL)
    {
        error_code = 8;
        return NULL;
    }
    for (int index = 0; index < letters_wide + 1; index++)
        str[index] = '\0';
    int digit = letters_wide;
    int str_index = 0;
    while (digit > 0)
    {
        int letter_index = calculate_letter_index(y_coordinate, digit, lower_boundary(NUM_LETTERS, digit - 1));
        str[str_index++] = ALPHABET[letter_index];
        y_coordinate -= (letter_index + 1) * pow(NUM_LETTERS, --digit);
    }

    return str;
}

/******************************************************************************************************************************
 * calculate_letter_digits:    Purpose: Calculates the number of letter-digits (eg, A is 1 digit, ZGM is 3 digits)            *
 *                                       the conversion of the given numerical input will result in.                          *
 *                             Parameters: int32_t number_to_convert -> the decimal number to be converted into letter-digits *
 *                             Return value: int -> the number of letter-digits the conversion will result in.                *
 *                             Side effects: none                                                                             *
 ******************************************************************************************************************************/
int calculate_letter_digits(int32_t number_to_convert)
{
    int digits = 1;
    while (number_to_convert >= lower_boundary(NUM_LETTERS, digits))
        digits++;
    return digits;
}

/**************************************************************************************************************************************
 * lower_boundary:    Purpose: Calculates the lower boundary of the coordinate range created by the current exponentiation iteration. *
 *                    Parameters: - int base -> the base to be exponentiated (in this program, the number of letters in the alphabet) *
 *                                - int power -> the largest power to be calculated (ie, the ordinal number of the current digit)     *
 *                    Return value: int32_t -> the lower boundary of the current range                                                *
 *                    Side effects: none                                                                                              *
 **************************************************************************************************************************************/
int32_t lower_boundary(int base, int power)
{
    int32_t sum = 0;
    while (power > 0)
        sum += pow(base, power--);
    return sum;
}

/**********************************************************************************************************************************************
 * calculate_letter_index():    Purpose: Calculates the index (within the alphabet, A-Z : 0-25) of the current digit in the letter coordinate.*
 *                              Parameters: - int32_t current_number -> the decimal number corresponding to the digit to be calculated        *
 *                                          - int current_digit -> the ordinal of the digit to be calculated                                  *
 *                                                                  (1 is far right, 2 is next to it, etc)                                    *
 *                                          - int32_t lower_boundary -> the lower boundary of                                                 *
 *                                                                       the coordinate range created by                                      *
 *                                                                       the previous exponentiation iteration                                *
 *                                                                       (aka, the return value of calling lower_boundary()                   *
 *                                                                       on the previous digit)                                               *
 *                              Return value: int -> the calculated alphabetical index                                                        *
 *                              Side effects: none                                                                                            *
 **********************************************************************************************************************************************/
int calculate_letter_index(int32_t current_number, int current_digit, int32_t lower_boundary)
{
    int counter = 0;
    while (current_number >= lower_boundary)
    {
        current_number -= pow(NUM_LETTERS, current_digit - 1);
        counter++;
    }
    return counter - 1;
}

/*****************************************************************************************
 * get_command():    Purpose: ONLY USE IN BUFFERED MODE                                                        *
 *                      Parameters (and the meaning of each):                            *
 *                      Return value:                                                    *
 *                      Side effects (such as modifying external variables,              *
 *                          printing to stdout, or exiting the program): - Modifies global variable "error_code"                *
 *****************************************************************************************/
int get_command(char *prompt)
{
    Command_C *root_c = NULL;
    int character = 0;
    int character_count = 0;

    (void) printf("%s", prompt);

    while ((character = getchar()) != '\n' && character != EOF)
    {
        Command_C *c = malloc(sizeof(Command_C));
        if (c == NULL)
        {
            error_code = 9;
            free_command(root_c);
            return -1;
        }
        c->c = character;
        c->next_c = NULL;

        if (root_c == NULL)
            root_c = c;
        else
        {
            Command_C *attach_point = root_c;
            while (attach_point->next_c != NULL)
                attach_point = attach_point->next_c;
            attach_point->next_c = c;
        }

        character_count++;
    }

    char *command = malloc(sizeof(char) * (character_count + 1));
    if (command == NULL)
    {
        error_code = 10;
        free_command(root_c);
        return -1;
    }
    Command_C *current = root_c;
    for (int index = 0; index < character_count; index++)
    {
        *(command + index) = current->c;
        current = current->next_c;
    }
    command[character_count] = '\0';
    free_command(root_c);

    int command_code = parse_command(command);
    free(command);

    return command_code;
}

void free_command(Command_C *root)
{
    if (root == NULL)
        return;
    free_command(root->next_c);
    free(root);
    return;
}

int parse_command(char *command)
{
    if (caseless_strcmp("help", command) || caseless_strcmp("h", command))
        return 1;
    else if (caseless_strcmp("quit", command) || caseless_strcmp("q", command))
        return 2;
    else if (caseless_strcmp("north", command) || caseless_strcmp("n", command))
        return 3;
    else if (caseless_strcmp("east", command) || caseless_strcmp("e", command))
        return 4;
    else if (caseless_strcmp("south", command) || caseless_strcmp("s", command))
        return 5;
    else if (caseless_strcmp("west", command) || caseless_strcmp("w", command))
        return 6;
    else
        return 0;
}

int caseless_strcmp(char *str1, char *str2)
{
    int n1 = strlen(str1), n2 = strlen(str2);
    if (n1 != n2)
        return 0;
    
    for (int char_index = 0; char_index < n1; char_index++)
    {
        if (tolower(str1[char_index]) != tolower(str2[char_index]))
            return 0;
    }
    return 1;
}

void obey_command(int command_code, Gamestate *g)
{
    switch (command_code)
    {
        default: error_code = 11; break;
        case 0: (void) printf("Unknown command. Type 'help' or 'h' for help.\n"), gobble_line(); break;
        case -1: /* Pass */ break;
        case 1: print_command_listing(); break;
        case 2: g->quit = true; break;
        case 3: move_cursor(NORTH, g); break;
        case 4: move_cursor(EAST, g); break;
        case 5: move_cursor(SOUTH, g); break;
        case 6: move_cursor(WEST, g); break;
    }
}

void print_command_listing(void)
{
    CLEAR_CONSOLE;
    (void) printf("----Valid Commands----\n"
                    "Function commands:\n"
                    "\t(H)elp: prints this listing\n"
                    "\t(Q)uit: returns to main menu\n"
                    "Movement commands:\n"
                    "\tNorth or N: moves the cursor up one space\n"
                    "\tEast or E: moves the cursor right one space\n"
                    "\tSouth or S: moves the cursor down one space\n"
                    "\tWest or W: moves the cursor left one space\n"
                    "\nCommands are not case-sensitive.\n");
    gobble_line();
    return;
}

void move_cursor(int cardinal_direction, Gamestate *g)
{
    //TODO: Adjust gamestate focus variable
    //TODO: Adjust display offset based on cardinal_direction

    int yesno = '\0';

    //Check if moving cursor would place if off the current layout:
    if (cardinal_direction == NORTH && g->current_cursor_focus->y_coordinate == 0) // NORTH LAYOUT EDGE
    {
        #ifdef FORCE_BUFFERED_MODE
            do
            {
                (void) printf("There is no row of rooms to the north. Would you like to shift the coordinate system and create a new row? (y/n) ");
                yesno = tolower(getchar()), gobble_line();
            } while (yesno != 'y' && yesno != 'n');
        #endif
        #ifdef UNIX
            //TODO
        #endif
        #ifdef WINDOWS
            //TODO
        #endif

        if (yesno == 'y')
        {
            add_row_north(g);
            // Move the display:
            g->display->y_offset--;
        }
    }
    else if (cardinal_direction == EAST && g->current_cursor_focus->x_coordinate == (g->current_map->width - 1)) // EAST LAYOUT EDGE
    {
        #ifdef FORCE_BUFFERED_MODE
            do
            {
                (void) printf("There is no column of rooms to the east. Would you like to create a new column? (y/n) ");
                yesno = tolower(getchar()), gobble_line();
            } while (yesno != 'y' && yesno != 'n');
        #endif
        #ifdef UNIX
            //TODO
        #endif
        #ifdef WINDOWS
            //TODO
        #endif

        if (yesno == 'y')
        {
            add_column_east(g);
            // Move the display:
            g->display->x_offset++;
        }
    }
    else if (cardinal_direction == SOUTH && g->current_cursor_focus->y_coordinate == (g->current_map->height - 1)) // SOUTH LAYOUT EDGE
    {
        #ifdef FORCE_BUFFERED_MODE
            do
            {
                (void) printf("There is no row of rooms to the south. Would you like to create a new row? (y/n) ");
                yesno = tolower(getchar()), gobble_line();
            } while (yesno != 'y' && yesno != 'n');
        #endif
        #ifdef UNIX
            //TODO
        #endif
        #ifdef WINDOWS
            //TODO
        #endif

        if (yesno == 'y')
        {
            add_row_south(g);
            // Move the display:
            g->display->y_offset++;
        }
    }
    else if (cardinal_direction == WEST && g->current_cursor_focus->x_coordinate == 0) // WEST LAYOUT EDGE
    {
        #ifdef FORCE_BUFFERED_MODE
            do
            {
                (void) printf("There is no column of rooms to the west. Would you like to shift the coordinate system and create a new column? (y/n) ");
                yesno = tolower(getchar()), gobble_line();
            } while (yesno != 'y' && yesno != 'n');
        #endif
        #ifdef UNIX
            //TODO
        #endif
        #ifdef WINDOWS
            //TODO
        #endif

        if (yesno == 'y')
        {
            add_column_west(g);
            // Move the display:
            g->display->x_offset--;
        }
    }
    //Check if moving cursor would place it off the current display:
    else if (cardinal_direction == NORTH && g->current_cursor_focus->y_coordinate == g->display->y_offset) // NORTH DISPLAY EDGE
    {
        // Move the display:
        g->display->y_offset--;
        // Move the cursor:
        g->current_cursor_focus = g->display->layout[g->current_cursor_focus->y_coordinate - 1][g->current_cursor_focus->x_coordinate];
    }
    else if (cardinal_direction == EAST && g->current_cursor_focus->x_coordinate == g->display->x_offset + (g->display->width - 1)) // EAST DISPLAY EDGE
    {
        // Move the display:
        g->display->x_offset++;
        // Move the cursor:
        g->current_cursor_focus = g->display->layout[g->current_cursor_focus->y_coordinate][g->current_cursor_focus->x_coordinate + 1];
    }
    else if (cardinal_direction == SOUTH && g->current_cursor_focus->y_coordinate == g->display->y_offset + (g->display->height - 1)) // SOUTH DISPLAY EDGE
    {
        // Move the display:
        g->display->y_offset++;
        // Move the cursor:
        g->current_cursor_focus = g->display->layout[g->current_cursor_focus->y_coordinate + 1][g->current_cursor_focus->x_coordinate];
    }
    else if (cardinal_direction == WEST && g->current_cursor_focus->x_coordinate == g->display->x_offset) // WEST DISPLAY EDGE
    {
        // Move the display:
        g->display->x_offset--;
        // Move the cursor:
        g->current_cursor_focus = g->display->layout[g->current_cursor_focus->y_coordinate][g->current_cursor_focus->x_coordinate - 1];
    }
    else // No need to adjust layout or display; just move the cursor:
    {
        switch (cardinal_direction)
        {
            default: error_code = 12; break;
            case NORTH:
                g->current_cursor_focus = g->display->layout[g->current_cursor_focus->y_coordinate - 1][g->current_cursor_focus->x_coordinate]; break;
            case EAST:
                g->current_cursor_focus = g->display->layout[g->current_cursor_focus->y_coordinate][g->current_cursor_focus->x_coordinate + 1]; break;
            case SOUTH:
                g->current_cursor_focus = g->display->layout[g->current_cursor_focus->y_coordinate + 1][g->current_cursor_focus->x_coordinate]; break;
            case WEST:
                g->current_cursor_focus = g->display->layout[g->current_cursor_focus->y_coordinate][g->current_cursor_focus->x_coordinate - 1]; break;
        }
    }

    return;
}

void add_row_north(Gamestate *g)
{
    Dimensions new_map_dim;
    new_map_dim.height = g->current_map->height + 1;
    new_map_dim.width = g->current_map->width;

    if (new_map_dim.height > MAX_COORDINATE)
    {
        (void) printf("Unable to comply: Adding new row would exceed maximum possible map size.\n");
        return;
    }

    Map *new_map = create_map(new_map_dim);
    if (error_code)
        return;

    Room ***new_layout = create_initial_layout(new_map);
    if (error_code)
    {
        free_layout(new_layout, new_map_dim.height);
        free_map(new_map);
        return;
    }

    // Transfer established Room properties:
    for (int32_t row = 0; row < g->current_map->height; row++)
    {
        for (int32_t column = 0; column < g->current_map->width; column++)
        {
            new_layout[row + 1][column]->exists = g->display->layout[row][column]->exists;
            new_layout[row + 1][column]->id_alias = g->display->layout[row][column]->id_alias;
            for (int cardinal_direction = NORTH; cardinal_direction < NUM_CARDINAL_DIRECTIONS; cardinal_direction++)
            {
                new_layout[row + 1][column]->exits[cardinal_direction] = g->display->layout[row][column]->exits[cardinal_direction];
            }
        }
    }

    free_layout(g->display->layout, g->current_map->height);
    free_map(g->current_map);
    g->display->layout = new_layout;
    g->current_map = new_map;

    return;
}

void add_column_east(Gamestate *g)
{
    Dimensions new_map_dim;
    new_map_dim.height = g->current_map->height;
    new_map_dim.width = g->current_map->width + 1;

    if (new_map_dim.width > MAX_COORDINATE)
    {
        (void) printf("Unable to comply: Adding new column would exceed maximum possible map size.\n");
        return;
    }

    Map *new_map = create_map(new_map_dim);
    if (error_code)
        return;

    Room ***new_layout = create_initial_layout(new_map);
    if (error_code)
    {
        free_layout(new_layout, new_map_dim.height);
        free_map(new_map);
        return;
    }

    // Transfer established Room properties:
    for (int32_t row = 0; row < g->current_map->height; row++)
    {
        for (int32_t column = 0; column < g->current_map->width; column++)
        {
            new_layout[row][column]->exists = g->display->layout[row][column]->exists;
            new_layout[row][column]->id_alias = g->display->layout[row][column]->id_alias;
            for (int cardinal_direction = NORTH; cardinal_direction < NUM_CARDINAL_DIRECTIONS; cardinal_direction++)
            {
                new_layout[row][column]->exits[cardinal_direction] = g->display->layout[row][column]->exits[cardinal_direction];
            }
        }
    }

    free_layout(g->display->layout, g->current_map->height);
    free_map(g->current_map);
    g->display->layout = new_layout;
    g->current_map = new_map;

    return;
}

void add_row_south(Gamestate *g)
{
    Dimensions new_map_dim;
    new_map_dim.height = g->current_map->height + 1;
    new_map_dim.width = g->current_map->width;

    if (new_map_dim.height > MAX_COORDINATE)
    {
        (void) printf("Unable to comply: Adding new row would exceed maximum possible map size.\n");
        return;
    }

    Map *new_map = create_map(new_map_dim);
    if (error_code)
        return;

    Room ***new_layout = create_initial_layout(new_map);
    if (error_code)
    {
        free_layout(new_layout, new_map_dim.height);
        free_map(new_map);
        return;
    }

    // Transfer established Room properties:
    for (int32_t row = 0; row < g->current_map->height; row++)
    {
        for (int32_t column = 0; column < g->current_map->width; column++)
        {
            new_layout[row][column]->exists = g->display->layout[row][column]->exists;
            new_layout[row][column]->id_alias = g->display->layout[row][column]->id_alias;
            for (int cardinal_direction = NORTH; cardinal_direction < NUM_CARDINAL_DIRECTIONS; cardinal_direction++)
            {
                new_layout[row][column]->exits[cardinal_direction] = g->display->layout[row][column]->exits[cardinal_direction];
            }
        }
    }

    free_layout(g->display->layout, g->current_map->height);
    free_map(g->current_map);
    g->display->layout = new_layout;
    g->current_map = new_map;

    return;
}

void add_column_west(Gamestate *g)
{
    Dimensions new_map_dim;
    new_map_dim.height = g->current_map->height;
    new_map_dim.width = g->current_map->width + 1;

    if (new_map_dim.width > MAX_COORDINATE)
    {
        (void) printf("Unable to comply: Adding new column would exceed maximum possible map size.\n");
        return;
    }

    Map *new_map = create_map(new_map_dim);
    if (error_code)
        return;

    Room ***new_layout = create_initial_layout(new_map);
    if (error_code)
    {
        free_layout(new_layout, new_map_dim.height);
        free_map(new_map);
        return;
    }

    // Transfer established Room properties:
    for (int32_t row = 0; row < g->current_map->height; row++)
    {
        for (int32_t column = 0; column < g->current_map->width; column++)
        {
            new_layout[row][column + 1]->exists = g->display->layout[row][column]->exists;
            new_layout[row][column + 1]->id_alias = g->display->layout[row][column]->id_alias;
            for (int cardinal_direction = NORTH; cardinal_direction < NUM_CARDINAL_DIRECTIONS; cardinal_direction++)
            {
                new_layout[row][column + 1]->exits[cardinal_direction] = g->display->layout[row][column]->exits[cardinal_direction];
            }
        }
    }

    free_layout(g->display->layout, g->current_map->height);
    free_map(g->current_map);
    g->display->layout = new_layout;
    g->current_map = new_map;

    return;
}

/*******************************************************************************************
 * save_map:    Purpose: Saves the given map to an external file in a bespoke file format; *
 *                       can be instructed to create new file or overwrite old file.       *
 *              Parameters: Map *savable_map -> the map to be saved to file.               *
 *              Return value: none                                                         *
 *              Side effects: - Creates or edits external files.                           *
 *                            - Prints to stdout                                           *
 *                            - Reads from stdin                                           *
 *******************************************************************************************/
void save_map(Map *savable_map)
{
    // TODO: Incorportate saving to new file and overriding old file.
    // TODO: Include ability to fetch list of files currently in system.
    // TODO: Similarly, or alternately, implement "Save" and "Save as..."
    return;
}

/**********************************************************************************************
 * free_map:    Purpose: Frees all allocated memory for the given map and its rooms.          *
 *              Parameters: Map *freeable_map -> The map to be freed.                         *
 *              Return value: none                                                            *
 *              Side effects: - Frees all memory associated with given map. Cannot be undone. *
 **********************************************************************************************/
void free_map(Map *freeable_map)
{
    free_rooms(freeable_map->root);
    free(freeable_map);
    return;
}

/*****************************************************************************************************
 * free_rooms:  Purpose: Recursively frees all allocated memory for the given room linked list node. *
 *              Parameters: Room *r -> The node to be recursively freed.                             *
 *              Return value: none                                                                   *
 *              Side effects: - Frees all memory associated with given node. Cannot be undone.       *
 *****************************************************************************************************/
void free_rooms(Room *r)
{
    // Base case:
    if (r == NULL)
        return;

    // Other cases:
    free_rooms(r->next_room);
    free(r);

    return;
}

//FUTURE TODOS:
// - Make height/width of editor adjustable for different sized monitors. Maybe implement a settings menu with defaults?
// - Make height/width of editor a variable based on internal reading of terminal window size (including if user re-sizes terminal midway through execution).
// - Add ability to turn off and back on the axis labels.
// - Add ability to declare the edge of the map and make rooms connect to the beginning of a different map on the same level, to make it more feasable to build large spaces by dividing them into separate chunks each on their own map.
// - Get rid of Room ids? Haven't seemed to use them thus far (2024-2-2). ID_alias would need to be renamed to just "label", or similar.
// - Combine the four add row/column functions into one, to reduce duplicate code.
// - Drastically reduce MAX_COORDINATE's size. No human would ever need a map that generous, and were one to be used, certain functions--like adding new rows/columns--would take a prohibitively long time (meaning DAYS of real time to create a duplicate map, for example).
// - Add option to switch between nesw controls and wasd controls.