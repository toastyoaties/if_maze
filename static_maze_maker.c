/****************************************************************************************************
 * Current goal: Program save/load
    Afterwards: Program non-buffered input.
 *                                                                                                  *
    Potential extras: Automated checking to see if maze is traversable; procedural map generation; demo map to play;
                        (OS-dependent:) code list all files in current directory during save/load.
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
#include <limits.h>

/* Preprocessing Directives (#define) */
#define CLEAR_CONSOLE (void) printf("\033[H\033[2J\033[3J") // ANSI escapes for clearing screen and scrollback.
#define NUM_CARDINAL_DIRECTIONS 4
#define MAX_DISPLAY_HEIGHT 20
#define MAX_DISPLAY_WIDTH 20
#define MAX_COORDINATE 321272405 // Because of the use of the pow() function, combined with the int32_t limit.
#define MAX_LETTER_COORDINATE "ZZZZZZ"
#define MAX_ID ((MAX_COORDINATE + 1) * (MAX_COORDINATE + 1))
#define NUM_LETTERS 26
#define ALPHABET "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define MIN_DISPLAY_COMMAND_LENGTH 11
#define MIN_JUMP_COMMAND_LENGTH 10
#define INT32_MAXIMUM_STRING STRINGIZE2(INT32_MAX)
#define STRINGIZE2(x) STRINGIZE(x)
#define STRINGIZE(x) #x

/* Type Definitions */
enum cardinal_directions
{
    NORTH,
    EAST,
    SOUTH,
    WEST,
};

enum movement_mode
{
    NESW,
    WASD,
};

typedef struct room
{
    int32_t y_coordinate;
    int32_t x_coordinate;
    bool exists;
    bool exits[4];
    struct room *next_room;
    char mark;
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

typedef struct settings
{
    int movement_mode;
    int max_display_height;
    int max_display_width;
} Settings;

typedef struct gamestate
{
    bool quit;
    bool saved;
    Display *display;
    Room *current_cursor_focus;
    Map *current_map;
    Settings *user_settings;
    Room *start;
    Room *end;
    char *current_filename;
} Gamestate;

/* Declarations of External Variables */
// none

/* Declarations of Global Variables */
int error_code = 0;

/* Prototypes for non-main functions */
void gobble_line(void);
Dimensions prompt_for_dimensions(void);
Map *create_map(Dimensions dim);
Room *make_room(int32_t y_coordinate, int32_t x_coordinate);
Map *load_map(long *fread_offset, char **file_to_load);
Gamestate *load_gamestate(long *fread_offset, char **file_to_load);
Map *edit_map(Map *editable_map, Gamestate *current_gamestate);
Room ***create_initial_layout(Map *map_to_display);
void free_layout(Room ***layout, int32_t height);
Display *initialize_display(Room ***layout_array, int32_t array_height, int32_t array_width, Room *root);
Settings *initialize_settings(void);
Gamestate *initialize_gamestate(Display *display, Map *current_map, Settings *defaults);
void print_display(Gamestate *g);
char *ystr(int32_t y_coordinate);
int calculate_letter_digits(int32_t number_to_convert);
int32_t lower_boundary(int base, int power);
int calculate_letter_index(int32_t current_number, int current_digit, int32_t lower_boundary);
int get_command(char *prompt, Gamestate *g, char mode);
void free_command(Command_C *root);
int parse_command(char *command, Gamestate *g);
int caseless_strcmp(char *str1, char *str2);
int display_strcmp(char *command, int32_t *user_display_rows, int32_t *user_display_columns);
int handle_display_command(Gamestate *g, int32_t user_rows, int32_t user_columns);
int jump_strcmp(char *command, char **letter_coordinate, char **number_coordinate);
int handle_jump_command(Gamestate *g, char *letter_coordinate, char *number_coordinate);
int32_t convert_letters_to_numbers(char *letter_coordinate);
int letter_position_in_alphabet(char letter);
void obey_command(int command_code, Gamestate *g);
void print_command_listing(Gamestate *g);
void move_cursor(int cardinal_direction, Gamestate *g);
void add_row_north(Gamestate *g);
void add_column_east(Gamestate *g);
void add_row_south(Gamestate *g);
void add_column_west(Gamestate *g);
void toggle_movement(Gamestate *g);
void mark(char mark, Gamestate *g);
void delete(Gamestate *g);
void undelete(Gamestate *g);
void open(Gamestate *g, int direction);
void close(Gamestate *g, int direction);
void remove_row_north(Gamestate *g);
void remove_column_east(Gamestate *g);
void remove_row_south(Gamestate *g);
void remove_column_west(Gamestate *g);
void save_gamestate(Gamestate *savable_gamestate);
void free_map(Map *freeable_map);
void free_rooms(Room *r);
void free_gamestate(Gamestate *g);
bool warn(Gamestate *g);

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
    // Loading is done in two variably sized, unequal parts.
    // This variable allows tracking of where first part ends so second part knows where to begin.
    // (This was a consequence of the greenness of the programmer when originally structuring the program.)
    long load_cursor = 0;
    // This variable allows tracking of loaded filename until the gamestate is loaded (for the same reason).
    char *file_to_load = NULL;

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
            case 2: free_map(edit_map(load_map(&load_cursor, &file_to_load), load_gamestate(&load_cursor, &file_to_load))); break;
            default: goto quit;
        }
        if (error_code) break;
    }
    // Exit program:
    quit:
    switch (error_code)
    {
        default: (void) printf("Encountered unexpected error. Error was assigned error code %d, but this code corresponds to no known error.\n", error_code); break;
        case 0: break; // no error
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
        case 14: (void) printf("Encountered error. Error code 14: Unable to allocate memory for settings.\n"); break;
        case 15: (void) printf("Encountered unexpected error. Error code 15: Received unknown mark code; cannot parse.\n"); break;
        case 16: (void) printf("Encountered unexpected error. Error code 16: Received unknown open direction; cannot parse.\n"); break;
        case 17: (void) printf("Encountered unexpected error. Error code 17: Received unknown close direction; cannot parse.\n"); break;
        case 18: (void) printf("Encountered error. Error code 18: Unable to allocate memory for user's display y-dimension string.\n"); break;
        case 19: (void) printf("Encountered error. Error code 19: Unable to allocate memory for user's display x-dimension string.\n"); break;
        case 20: (void) printf("Encountered unexpected error. Error code 20: Passed character not in alphabet.\n"); break;
        case 21: (void) printf("Encountered error. Error code 21: Unable to allocate memory for normalized version of user's jump command.\n"); break;
        case 22: (void) printf("Encountered error. Error code 22: Unable to allocate memory for letter coordinates from user's jump command.\n"); break;
        case 23: (void) printf("Encountered error. Error code 23: Unable to allocate memory for number coordinates from user's jump command.\n"); break;
        case 24: (void) printf("Encountered unexpected error. Error code 24: Received unknown warning response code; cannot parse.\n"); break;
        case 25: (void) printf("Encountered unexpected error. Error code 25: Received unknown mode switch for command processing; cannot parse.\n"); break;
        case 26: (void) printf("Encountered error. Error code 26: Failed to properly close file after checking for presence of file with duplicate name.\n"); break;
        case 27: (void) printf("Encountered error. Error code 27: Failed to properly write to savefile.\n"); break;
        case 28: (void) printf("Encountered error. Error code 28: Failed to properly close savefile.\n"); break;
        case 29: (void) printf("Encountered error. Error code 29: Failed both to properly write to savefile and to properly close savefile.\n"); break;
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
    for (int32_t y = 0; y < created_map->height; y++)
    {
        for (int32_t x = 0; x < created_map->width; x++)
        {
            Room *r = make_room(y, x);
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
 *               Return value: Room * -> a pointer to the new room                           *
 *               Side effects: - Allocates memory.                                           *
 *                             - Edits global variable "error_code".                         *
 *********************************************************************************************/
Room *make_room(int32_t y_coordinate, int32_t x_coordinate)
{
    Room *r = malloc(sizeof(Room));
    if (r == NULL)
    {
        error_code = 5;
        return NULL;
    }

    r->y_coordinate = y_coordinate, r->x_coordinate = x_coordinate;
    r->exists = true, r->next_room = NULL;
    for (int cardinal_direction = NORTH; cardinal_direction < NUM_CARDINAL_DIRECTIONS; cardinal_direction++)
    {
        r->exits[cardinal_direction] = 0;
    }
    r->mark = 0;

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
Map *load_map(long *fread_offset, char **file_to_load)
{
    //TODO
    //  ...as part of the function, perform validation on the map file (for example, that it hasn't been edited so as to expand past MAX_COORDINATE, etc)
    // This function will need to run malloc for maps & rooms, or use the constructor subroutines and then re-assign properties from the loaded file to the malloc'd variables.

    //According to the C standard (C90 §6.3.2.2, C99 §6.5.2.2), the order in which function arguments are evaluated is unspecified.
    //Therefore, it can't be certain whether load_gamestate() or load_map() will run first,
    //meaning the user might have already specified which file to load during load_gamestate().
    if (*file_to_load == NULL)
    {
    }

    Map *loaded_map = NULL;
    return loaded_map;
}

Gamestate *load_gamestate(long *fread_offset, char **file_to_load)
{
    //TODO
    // as part of the function, perform validation on the gameplay file (if that makes sense when the time comes)
    // This function will need to run malloc for display & layout & gamestate, or use the constructor subroutines and then re-assign properties from the loaded file to the malloc'd variables.

    //According to the C standard (C90 §6.3.2.2, C99 §6.5.2.2), the order in which function arguments are evaluated is unspecified.
    //Therefore, it can't be certain whether load_gamestate() or load_map() will run first,
    //meaning fread_offset might not have been altered appropriately yet.
    if (*fread_offset == 0)
    {
    }
    if (*file_to_load == NULL)
    {
    }

    // ERASE ".ifmap" FROM END OF FILENAME BEFORE STORING IN LOADED_GAMESTATE->CURRENT_FILENAME!

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
 *                            - Calls save_gamestate, which edits external files.                 *
 *                            - Prints to stdout.                                           *
 *                            - Reads from stdin.                                           *
 *                            - Modifies any and all data associated with passed map.       *
 ********************************************************************************************/
Map *edit_map(Map *editable_map, Gamestate *current_gamestate)
{
    if (error_code) return editable_map;

    Gamestate *gamestate;

    if (current_gamestate == NULL) // If starting a new map, not loading one:
    {
        // Temp variables used for initialization purposes only:
        Room ***layout;
        Display *display;
        Settings *settings;

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

        settings = initialize_settings();
        if (error_code)
        {
            free_layout(layout, editable_map->height);
            free(display);
        }

        gamestate = initialize_gamestate(display, editable_map, settings);
        if (error_code)
        {
            free_layout(layout, editable_map->height);
            free(display);
            free(settings);
        }
        // Now that gamestate has been created & initialized, the layout/display/editable_map variables will no longer be used.
        // For memory-management reasons, all access to displays/layouts/maps will be accomplished only via the gamestate structure.
        // Otherwise, when new layouts/maps are created/destroyed during the course of subroutines, these old variables will still contain the
        // *old* pointer addresses rather than the various newly-created/updated ones. Free()-ing is much easier if I can free everything via
        // the gamestate variable instead. (The map is a special case, as the structure of the main menu requires the map to be passed to a
        // free()-ing subroutine *after* this edit_map subroutine returns, so I will simply re-assign the editable_map variable to the newest
        // map via gamestate just before freeing gamestate and returning editable_map. This weirdness is simply a consequence of my never having
        // built a program this complex before; in future programs with similar goals, I will likely try to center all the structure around a
        // gamestate from the very start, instead of centering around, say, a map, as here.)
    }
    else // If continuing a loaded file:
    {
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
            free_layout(gamestate->display->layout, gamestate->current_map->height);
            free(gamestate->display);
            free(gamestate->user_settings);
            editable_map = gamestate->current_map; // Re-establish map as its own variable so as to return and free it even once gamestate is already freed.
            free_gamestate(gamestate);
            return editable_map;
        }

        #ifdef FORCE_BUFFERED_MODE
            obey_command(get_command("Enter command:\n>", gamestate, 'c'), gamestate);
            if (error_code)
            {
                free_layout(gamestate->display->layout, gamestate->current_map->height);
                free(gamestate->display);
                free(gamestate->user_settings);
                editable_map = gamestate->current_map; // Re-establish map as its own variable so as to return and free it even once gamestate is already freed.
                free_gamestate(gamestate);
                return editable_map;
            }
            if (gamestate->quit && !gamestate->saved)
                if (warn(gamestate))
                    save_gamestate(gamestate);
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
    // Allow commands for expanding grid by adding new individual rooms
    //      ...but ensure that this never expands past the MAX_COORDINATE limit.
    // Allow commands for deleting individual rooms
    //      ...but ensure the row/column count stays above zero
    // Allow commands for retracting grid by subtracting x rows or columns
    //      ...but ensure the row/column count stays above zero
    // Allow commands for establishing entrances and exits to maze (including interior ones)
    // Allow commands for scrolling view of grid cardinally (in this case, meaning up, down, left, right)
    // Let attempts at moving the cursor past the edges of the grid cause the grid to scroll (if there are rooms in that direction).
    // Allow commands for saving maze to file and reading maze from file
    // Allow commands for translating entire map plus or minus x x-values or y y-values
    // Allow commands to establish critical path (in separate ANSI color)
    // Allow command to highlight which rooms cannot be reached
    // Allow command to determine whether critical path exists


    //TODO: Allow saving map before returning (returning leads to freeing--aka losing--map from memory)
    //TODO: Warns when about to return without saving
    free_layout(gamestate->display->layout, gamestate->current_map->height);
    free(gamestate->display);
    free(gamestate->user_settings);
    editable_map = gamestate->current_map; // Re-establish map as its own variable so as to return and free it even once gamestate is already freed.
    free_gamestate(gamestate);
    return editable_map;
}

/**********************************************************************************************************
 * create_initial_layout:     Purpose: Allocates room for, and initializes,                               *
 *                                     a 2D array containing the addresses of rooms to display            *
 *                                     as visualized map during editing.                                  *
 *                            Parameters: Map *map_to_display -> the map data to create the array from.   *
 *                            Return value: Room *** -> a pointer to the array                            *
 *                            Side effects: - allocates memory                                            *
 *                                          - edits global variable "error_code"                          *
 **********************************************************************************************************/
Room ***create_initial_layout(Map *map_to_display)
{
    //Create 2D grid to store rooms:
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

Settings *initialize_settings(void)
{
    Settings *s = malloc(sizeof(Settings));
    if (s == NULL)
    {
        error_code = 14;
        return NULL;
    }

    s->movement_mode = NESW;
    s->max_display_height = MAX_DISPLAY_HEIGHT;
    s->max_display_width = MAX_DISPLAY_WIDTH;

    return s;
}

/*****************************************************************************************
 * name_of_function:    Purpose:                                                         *
 *                      Parameters (and the meaning of each):                            *
 *                      Return value:                                                    *
 *                      Side effects (such as modifying external variables,              *
 *                          printing to stdout, or exiting the program):                 *
 *****************************************************************************************/
Gamestate *initialize_gamestate(Display *display, Map *current_map, Settings *defaults)
{
    Gamestate *g = malloc(sizeof(Gamestate));
    if (g == NULL)
    {
        error_code = 13;
        return NULL;
    }

    g->quit = false;
    g->saved = false;
    g->display = display;
    g->current_map = current_map;
    g->current_cursor_focus = display->layout[0][0];
    g->user_settings = defaults;
    g->start = g->end = NULL;
    g->current_filename = NULL;

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

    // Reset display to map size or maximum size, whichever is smaller:
    g->display->height = g->current_map->height < g->user_settings->max_display_height ? g->current_map->height : g->user_settings->max_display_height;
    g->display->width = g->current_map->width < g->user_settings->max_display_width ? g->current_map->width : g->user_settings->max_display_width;

    // Shrink offset so that it won't exceed map size when combined with display:
    if (g->display->height + g->display->y_offset > g->current_map->height)
        g->display->y_offset = g->current_map->height - g->display->height;
    if (g->display->width + g->display->x_offset > g->current_map->width)
        g->display->x_offset = g->current_map->width - g->display->width;

    // Reset cursor if off display:
    int new_cursor_y = 0, new_cursor_x = 0;
    if (g->current_cursor_focus->y_coordinate > g->display->height + g->display->y_offset - 1) // if below screen
        new_cursor_y = g->display->height + g->display->y_offset - 1;
    else if (g->current_cursor_focus->y_coordinate < g->display->y_offset) // if above screen
        new_cursor_y = g->display->y_offset;
    else
        new_cursor_y = g->current_cursor_focus->y_coordinate;
    if (g->current_cursor_focus->x_coordinate > g->display->width + g->display->x_offset - 1) // if off-screen to the right
        new_cursor_x = g->display->width + g->display->x_offset - 1;
    else if (g->current_cursor_focus->x_coordinate < g->display->x_offset) // if off-screen to the left
        new_cursor_x = g->display->x_offset;
    else
        new_cursor_x = g->current_cursor_focus->x_coordinate;
    g->current_cursor_focus = g->display->layout[new_cursor_y][new_cursor_x];

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
    int assumed_terminal_width_in_cols = g->user_settings->max_display_width * min_cell_width;
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
                if (current->exists && current->exits[WEST])
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
            else if (current->mark)
                (void) printf("%c", current->mark);
            else
                (void) printf(" ");
            if (current->exists)
                (void) printf(")");
            else
                (void) printf(" ");

            // Print either right hyphens or spaces depending on west exit per room
            //      (this code assumes a west exit always corresponds with an east exit to the right):
            for (int hyphen = 0; hyphen < right_hyphens; hyphen++)
                if (current->exists && current->exits[EAST])
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
int get_command(char *prompt, Gamestate *g, char mode)
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

    if (mode == 'c')
    {
        int command_code = parse_command(command, g);
        free(command);
        return command_code;
    }
    else if (mode == 's')
    {
        g->current_filename = command;
        return 0;
    }
    else
    {
        error_code = 25;
        free(command);
        return -1;
    }
}

void free_command(Command_C *root)
{
    if (root == NULL)
        return;
    free_command(root->next_c);
    free(root);
    return;
}

int parse_command(char *command, Gamestate *g)
{
    // Initialize variables necessary for parsing:
    int32_t user_display_rows = 0, user_display_columns = 0; // Needed to parse user's display commands.
    char *letter_coordinate_holder = NULL, *number_coordinate_holder = NULL; // Needed to parse user's jump commands.

    // String comparisons and code returns:
    if (caseless_strcmp("help", command) || caseless_strcmp("h", command))
        return 1;
    else if (caseless_strcmp("quit", command) || caseless_strcmp("q", command))
        return 2;
    else if (g->user_settings->movement_mode == NESW ? caseless_strcmp("north", command) || caseless_strcmp("n", command) : caseless_strcmp("up", command) || caseless_strcmp("w", command))
        return g->saved = false, 3;
    else if (g->user_settings->movement_mode == NESW ? caseless_strcmp("east", command) || caseless_strcmp("e", command) : caseless_strcmp("right", command) || caseless_strcmp("d", command))
        return g->saved = false, 4;
    else if (g->user_settings->movement_mode == NESW ? caseless_strcmp("south", command) || caseless_strcmp("s", command) : caseless_strcmp("down", command) || caseless_strcmp("s", command))
        return g->saved = false, 5;
    else if (g->user_settings->movement_mode == NESW ? caseless_strcmp("west", command) || caseless_strcmp("w", command) : caseless_strcmp("left", command) || caseless_strcmp("a", command))
        return g->saved = false, 6;
    else if (caseless_strcmp("toggle movement", command))
        return g->saved = false, 7;
    else if (caseless_strcmp("mark start", command))
        return g->saved = false, 8;
    else if (caseless_strcmp("mark end", command))
        return g->saved = false, 9;
    else if (caseless_strcmp("unmark", command))
        return g->saved = false, 10;
    else if (caseless_strcmp("delete", command))
        return g->saved = false, 11;
    else if (caseless_strcmp("undelete", command))
        return g->saved = false, 12;
    else if (caseless_strcmp("open up", command) || caseless_strcmp("open n", command) || caseless_strcmp("open north", command))
        return g->saved = false, 13;
    else if (caseless_strcmp("open right", command) || caseless_strcmp("open e", command) || caseless_strcmp("open east", command))
        return g->saved = false, 14;
    else if (caseless_strcmp("open down", command) || caseless_strcmp("open s", command) || caseless_strcmp("open south", command))
        return g->saved = false, 15;
    else if (caseless_strcmp("open left", command) || caseless_strcmp("open w", command) || caseless_strcmp("open west", command))
        return g->saved = false, 16;
    else if (caseless_strcmp("close up", command) || caseless_strcmp("close n", command) || caseless_strcmp("close north", command))
        return g->saved = false, 17;
    else if (caseless_strcmp("close right", command) || caseless_strcmp("close e", command) || caseless_strcmp("close east", command))
        return g->saved = false, 18;
    else if (caseless_strcmp("close down", command) || caseless_strcmp("close s", command) || caseless_strcmp("close south", command))
        return g->saved = false, 19;
    else if (caseless_strcmp("close left", command) || caseless_strcmp("close w", command) || caseless_strcmp("close west", command))
        return g->saved = false, 20;
    else if (caseless_strcmp("add row north", command) || caseless_strcmp("add row n", command) || caseless_strcmp("add n", command))
        return g->saved = false, 21;
    else if (caseless_strcmp("add column east", command) || caseless_strcmp("add column e", command) || caseless_strcmp("add e", command))
        return g->saved = false, 22;
    else if (caseless_strcmp("add row south", command) || caseless_strcmp("add row s", command) || caseless_strcmp("add s", command))
        return g->saved = false, 23;
    else if (caseless_strcmp("add column west", command) || caseless_strcmp("add column w", command) || caseless_strcmp("add w", command))
        return g->saved = false, 24;
    else if (caseless_strcmp("remove row north", command) || caseless_strcmp("remove row n", command) || caseless_strcmp("remove n", command) || caseless_strcmp("rem n", command))
        return g->saved = false, 25;
    else if (caseless_strcmp("remove column east", command) || caseless_strcmp("remove column e", command) || caseless_strcmp("remove e", command) || caseless_strcmp("rem e", command))
        return g->saved = false, 26;
    else if (caseless_strcmp("remove row south", command) || caseless_strcmp("remove row s", command) || caseless_strcmp("remove s", command) || caseless_strcmp("rem s", command))
        return g->saved = false, 27;
    else if (caseless_strcmp("remove column west", command) || caseless_strcmp("remove column w", command) || caseless_strcmp("remove w", command) || caseless_strcmp("rem w", command))
        return g->saved = false, 28;
    else if (display_strcmp(command, &user_display_rows, &user_display_columns))
        return g->saved = false, handle_display_command(g, user_display_rows, user_display_columns);
    else if (jump_strcmp(command, &letter_coordinate_holder, &number_coordinate_holder))
        return g->saved = false, handle_jump_command(g, letter_coordinate_holder, number_coordinate_holder);
    else
        return error_code ? -1 : 0;
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

int display_strcmp(char *command, int32_t *user_display_rows, int32_t *user_display_columns)
{
    // "display #x#" is at least MIN_DISPLAY_COMMAND_LENGTH chars long:
    int n = strlen(command);
    if (n < MIN_DISPLAY_COMMAND_LENGTH)
        return 0;

    // Make sure command matches the necessary model (while also preparing to convert user dimensions to ints):
    //      First section:
    char *needed_start = "display ";
    int n2 = strlen(needed_start);
    int index = 0, y_index = 0, x_index = 0;
    bool leading_zero = false;
    for (; index < n2; index++)
    {
        if (needed_start[index] != tolower(command[index]))
            return 0;
    }
    //      Second section:
    if (!isdigit(command[index]))
        return 0;
    if (command[index] == '0')
        leading_zero = true;
    int y_dimension_len = 0, x_dimension_len = 0;
    y_index = index;
    while (isdigit(command[index]))
    {
        y_dimension_len++;
        index++;
    }
    if (leading_zero && y_dimension_len > 1)
        return 0;
    leading_zero = false;
    //      Third section:
    if (tolower(command[index++]) != 'x')
        return 0;
    //      Fourth section:
    if (!isdigit(command[index]))
        return 0;
    if (command[index] == '0')
        leading_zero = true;
    x_index = index;
    while (isdigit(command[index]))
    {
        x_dimension_len++;
        index++;
    }
    if (leading_zero && x_dimension_len > 1)
        return 0;
    //      Fifth section:
    if (command[index] != '\0')
        return 0;

    // Capture numbers by converting from char * to int:
    char *y_dim_chars = malloc(sizeof(char) * (y_dimension_len + 1));
    if (y_dim_chars == NULL)
    {
        error_code = 18;
        return 0;
    }
    char *x_dim_chars = malloc(sizeof(char) * (x_dimension_len + 1));
    if (x_dim_chars == NULL)
    {
        error_code = 19;
        free(y_dim_chars);
        return 0;
    }

    // Convert while ensuring user submitted numbers are not too large to fit in an int32:
    int i = 0;
    for (; i < y_dimension_len; i++)
        y_dim_chars[i] = command[y_index++];
    y_dim_chars[i] = '\0';

    for (i = 0; i < x_dimension_len; i++)
        x_dim_chars[i] = command[x_index++];
    x_dim_chars[i] = '\0';

    bool too_big = false;
    int max_len = strlen(INT32_MAXIMUM_STRING);
    int y_len = strlen(y_dim_chars), x_len = strlen(x_dim_chars);
    if (y_len > max_len)
        *user_display_rows = INT32_MAX;
    else if (y_len < max_len)
    {
        // Convert input:
        int32_t typecast = 0;
        for (i = 0; i < y_len; i++)
            typecast += (y_dim_chars[(y_len - 1) - i] - '0') * (pow(10, i));
        *user_display_rows = typecast;
    }
    else // same length
    {
        for (i = 0; i < max_len; i++)
            if (y_dim_chars[i] - '0' > INT32_MAXIMUM_STRING[i] - '0')
            {
                *user_display_rows = INT32_MAX;
                too_big = true;
                break;
            }
            /*
             * Note on the prior "if" statement:
             * For those who are (like I was) concerned about portability between character sets (in other words, for anyone worried
             * about whether their C code will run correctly even on systems that do not use ASCII character encoding),
             * this technique of converting a character of an integer into an actual integer (ie, converting '7' to 7, etc)
             * is, in fact, portable in all versions of C since C89 (I haven't checked either edition of K&R C yet, so it might have started earlier).
             * According to the C89 standard (§2.2.1 in ANSI X3.159-1989, §5.2.1 in ANSI/IS0 9899-1990):
             * "In both the source and execution basic character sets,
             * the value of each character after 0 in the above list of decimal digits [0 1 2 3 4 5 6 7 8 9]
             * shall be one greater than the value of the previous".
             * It's worth noting that the C standard does NOT guaranteed that the *alphabetical* characters
             * will be contiguous, only the digit characters (meaning "<char> - '0'" is portable, but "<char> - 'A'" isn't always).
             */
        if (!too_big)
        {
            // Convert input:
            int32_t typecast = 0;
            for (i = 0; i < y_len; i++)
                typecast += (y_dim_chars[(y_len - 1) - i] - '0') * (pow(10, i));
            *user_display_rows = typecast;
        }
    }
    too_big = false;
    if (x_len > max_len)
        *user_display_columns = INT32_MAX;
    else if (x_len < max_len)
    {
        // Convert input:
        int32_t typecast = 0;
        for (i = 0; i < x_len; i++)
            typecast += (x_dim_chars[(x_len - 1) - i] - '0') * (pow(10, i));
        *user_display_columns = typecast;
    }
    else // same length
    {
        for (i = 0; i < max_len; i++)
            if (x_dim_chars[i] - '0' > INT32_MAXIMUM_STRING[i] - '0')
            {
                *user_display_columns = INT32_MAX;
                too_big = true;
                break;
            }
        if (!too_big)
        {
            // Convert input:
            int32_t typecast = 0;
            for (i = 0; i < x_len; i++)
                typecast += (x_dim_chars[(x_len - 1) - i] - '0') * (pow(10, i));
            *user_display_columns = typecast;
        }
    }

    free(y_dim_chars);
    free(x_dim_chars);

    return 1;
}

int handle_display_command(Gamestate *g, int32_t user_rows, int32_t user_columns)
{
    // Check if 0s:
    if (user_rows == 0 || user_columns == 0)
        return 29;

    // Shrink user_rows and user_columns to be within MAX_COORDINATE:
    if (user_rows > MAX_COORDINATE)
        user_rows = MAX_COORDINATE;
    if (user_columns > MAX_COORDINATE)
        user_columns = MAX_COORDINATE;

    g->user_settings->max_display_height = user_rows, g->user_settings->max_display_width = user_columns;
    return -1;
}

int jump_strcmp(char *command, char **letter_coordinate, char **number_coordinate)
{
    int n = strlen(command);
    if (n < MIN_JUMP_COMMAND_LENGTH)
        return 0;
    char *lower_command = malloc(sizeof(char) * n);
    if (lower_command == NULL)
    {
        error_code = 21;
        return 0;
    }

    for (int i = 0; i < n; i++)
        *(lower_command + i) = tolower(command[i]);

    if (strncmp("jump to ", lower_command, 8) != 0)
        return 0;

    free(lower_command);

    bool letter_slot = true;
    int letters = 0, numbers = 0;
    for (int i = 8; i < n; i++)
    {
        if (letter_slot)
        {
            if (!isalpha(command[i]))
            {
                if (!letters)
                    return 0;
                else if (isdigit(command[i]))
                {
                    letter_slot = false;
                    numbers++;
                }
                else
                    return 0;
            }
            else
                letters++;

        }
        else
        {
            if (!isdigit(command[i]))
                return 0;
            else
                numbers++;
        }
    }
    if (!numbers)
        return 0;
    
    *letter_coordinate = malloc(sizeof(**letter_coordinate) * (letters + 1));
    if (*letter_coordinate == NULL)
    {
        error_code = 22;
        return 0;
    }
    *number_coordinate = malloc(sizeof(**number_coordinate) * (numbers + 1));
    if (*number_coordinate == NULL)
    {
        free(*letter_coordinate);
        error_code = 23;
        return 0;
    }

    int i;
    for (i = 8; i < 8 + letters; i++)
        *(*letter_coordinate + (i - 8)) = toupper(command[i]);
    *(*letter_coordinate + (i - 8)) = '\0';
    for (i = 8 + letters; i < 8 + letters + numbers; i++)
        *(*number_coordinate + (i - 8 - letters)) = command[i];
    *(*number_coordinate + (i - 8 - letters)) = '\0';

    // Ensure there are no leading zeroes:
    if (strlen(*number_coordinate) > 1 && *number_coordinate[0] == '0')
    {
        free(*letter_coordinate);
        free(*number_coordinate);
        return 0;
    }

    return 1;
}

int handle_jump_command(Gamestate *g, char *letter_coordinate, char *number_coordinate)
{
    bool invalid_number_coordinate = false, invalid_letter_coordinate = false;
    int32_t converted_number_coordinate = 0, converted_letter_coordinate = 0;

    // Make sure number_coordinate can fit in an int32:
    bool too_big = false;
    int max_len = strlen(INT32_MAXIMUM_STRING);
    int num_len = strlen(number_coordinate);
    if (num_len > max_len)
        invalid_number_coordinate = true;
    else if (num_len < max_len)
    {
        // Convert input:
        int32_t typecast = 0;
        for (int i = 0; i < num_len; i++)
            typecast += (number_coordinate[(num_len - 1) - i] - '0') * (pow(10, i));
        converted_number_coordinate = typecast;
    }
    else // same length
    {
        for (int i = 0; i < max_len; i++)
            if (number_coordinate[i] - '0' > INT32_MAXIMUM_STRING[i] - '0')
            {
                invalid_number_coordinate = true;
                break;
            }
        if (!invalid_number_coordinate)
        {
            // Convert input:
            int32_t typecast = 0;
            for (int i = 0; i < num_len; i++)
                typecast += (number_coordinate[(num_len - 1) - i] - '0') * (pow(10, i));
            converted_number_coordinate = typecast;
        }
    }

    max_len = strlen(MAX_LETTER_COORDINATE);
    int x_len = strlen(letter_coordinate);
    if (x_len > max_len)
        invalid_letter_coordinate = true;
    else
        converted_letter_coordinate = convert_letters_to_numbers(letter_coordinate);

    free(number_coordinate);
    free(letter_coordinate);

    // Check if coordinates are on map:
    if (converted_number_coordinate > g->current_map->width - 1)
        invalid_number_coordinate = true;
    if (converted_letter_coordinate > g->current_map->height - 1)
        invalid_letter_coordinate = true;

    // Return codes for invalid coordinates:
    if (invalid_letter_coordinate && invalid_number_coordinate)
        return 30;
    else if (invalid_letter_coordinate)
        return 31;
    else if (invalid_number_coordinate)
        return 32;

    // Assign cursor to new valid coordinates:
    g->current_cursor_focus = g->display->layout[converted_letter_coordinate][converted_number_coordinate];

    // Change display offsets to reach new cursor:
    if (g->current_cursor_focus->y_coordinate < g->display->y_offset)
        g->display->y_offset = g->current_cursor_focus->y_coordinate;
    else if (g->current_cursor_focus->y_coordinate > g->display->y_offset + (g->display->height - 1))
        g->display->y_offset = (g->display->height - 1) + g->current_cursor_focus->y_coordinate;
    if (g->current_cursor_focus->x_coordinate < g->display->x_offset)
        g->display->x_offset = g->current_cursor_focus->x_coordinate;
    else if (g->current_cursor_focus->x_coordinate > g->display->x_offset + (g->display->width - 1))
        g->display->x_offset = (g->display->width - 1) + g->current_cursor_focus->x_coordinate;

    return -1;
}

int32_t convert_letters_to_numbers(char *letter_coordinate)
{
    int n = strlen(letter_coordinate);
    int32_t sum = 0;
    for (int i = 0, digit = n; i < n; i++, digit--)
    {
        int letter_position = letter_position_in_alphabet(letter_coordinate[i]);
        if (letter_position == -1)
        {
            error_code = 20;
            return -1;
        }
        sum += letter_position * pow(NUM_LETTERS, digit - 1);
    }

    return --sum; // Decrement to start from zero.
}

int letter_position_in_alphabet(char letter) // Positions start at 1
{
    for (int i = 0; i < NUM_LETTERS; i++)
    {
        if (ALPHABET[i] == letter)
            return ++i;
    }

    return -1; // Not in alphabet.
}

void obey_command(int command_code, Gamestate *g)
{
    switch (command_code)
    {
        default: error_code = 11; break;
        case 0: (void) printf("Unknown command. Type 'help' or 'h' for help.\n"), gobble_line(); break;
        case -1: /* Pass */ break;
        case 1: print_command_listing(g); break;
        case 2: g->quit = true; break;
        case 3: move_cursor(NORTH, g); break;
        case 4: move_cursor(EAST, g); break;
        case 5: move_cursor(SOUTH, g); break;
        case 6: move_cursor(WEST, g); break;
        case 7: toggle_movement(g); break;
        case 8: mark('S', g); break;
        case 9: mark('E', g); break;
        case 10: mark(0, g); break;
        case 11: delete(g); break;
        case 12: undelete(g); break;
        case 13: open(g, NORTH); break;
        case 14: open(g, EAST); break;
        case 15: open(g, SOUTH); break;
        case 16: open(g, WEST); break;
        case 17: close(g, NORTH); break;
        case 18: close(g, EAST); break;
        case 19: close(g, SOUTH); break;
        case 20: close(g, WEST); break;
        case 21: add_row_north(g); break;
        case 22: add_column_east(g); break;
        case 23: add_row_south(g); break;
        case 24: add_column_west(g); break;
        case 25: remove_row_north(g); break;
        case 26: remove_column_east(g); break;
        case 27: remove_row_south(g); break;
        case 28: remove_column_west(g); break;
        case 29: (void) printf("Display window must be at least 1x1.\n"), gobble_line(); break;
        case 30: (void) printf("Unable to jump: the given y- and x- coordinates are off the map.\n"), gobble_line(); break;
        case 31: (void) printf("Unable to jump: the given y-coordinate is off the map.\n"), gobble_line(); break;
        case 32: (void) printf("Unable to jump: the given x-coordinate is off the map.\n"), gobble_line(); break;
    }
}

void print_command_listing(Gamestate *g)
{
    CLEAR_CONSOLE;
    (void) printf("----Valid Commands----\n"
                    "Function commands:\n"
                    "\t(H)elp: prints this listing\n"
                    "\t(Q)uit: returns to main menu\n"
                    "Room editing commands:\n"
                    "\tDelete: removes current room from map\n"
                    "\tUndelete: restores deleted room to map\n"
                    "\tMark start: marks current room as the maze start\n"
                    "\tMark end: marks current room as the maze end\n"
                    "\tUnmark: removes start/end mark from current room\n"
                    "\tOpen <direction>: connects current room with the room in <direction>\n"
                    "\tClose <direction>: disconnects current room with the room in <direction>\n"
                    "\t\t<direction> can be up/right/down/left or NESW\n"
                    "Map editing commands:\n"
                    "\tAdd row north / south (or add n/s): Creates a new map row in the specified direction\n"
                    "\tAdd column east / west (or add e/w): Creates a new map column in the specified direction\n"
                    "\tRemove row north / south (or rem n/s): Removes the furthest map row in the specified direction\n"
                    "\tRemove column east / west (or rem e/w): Removes the furthest map column in the specified direction\n"
                    "Settings commands:\n"
                    "\tDisplay <rows>x<columns>: Adjusts the maximum display size\n");
    if (g->user_settings->movement_mode == NESW)
    {
        (void) printf(
                        "\tToggle Movement: re-maps movement commands to WASD\n"
                        "Movement commands:\n"
                        "\t(N)orth: moves the cursor up one space\n"
                        "\t(E)ast: moves the cursor right one space\n"
                        "\t(S)outh: moves the cursor down one space\n"
                        "\t(W)est: moves the cursor left one space\n");
    }
    else
    {
        (void) printf(
                        "\tToggle Movement: re-maps movement commands to NESW\n"
                        "Movement commands:\n"
                        "\tUp (W): moves the cursor up one space\n"
                        "\tLeft (A): moves the cursor left one space\n"
                        "\tDown (S): moves the cursor down one space\n"
                        "\tRight (D): moves the cursor right one space\n");
    }
    (void) printf("\tJump to <letter_coordinate><number_coordinate>: Moves cursor to specified room\n"
                    "\nCommands are not case-sensitive.\n"
                    "Zero must be written as a single digit; numbers greater than zero must not include leading zeroes.\n");
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
            // Move the display if display cannot grow:
            if (g->display->width == g->user_settings->max_display_width && g->current_cursor_focus->x_coordinate == (g->display->width - 1) + g->display->x_offset)
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
            // Move the display if display cannot grow:
            if (g->display->height == g->user_settings->max_display_height && g->current_cursor_focus->y_coordinate == (g->display->height - 1) + g->display->y_offset)
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
    // Store cursor position, adjusted inward if on southern end of map (to prevent cursor falling out of display after row is added):
    int32_t cursor_y = g->current_cursor_focus->y_coordinate == (g->display->height - 1) + g->display->y_offset ? g->current_cursor_focus->y_coordinate - 1 : g->current_cursor_focus->y_coordinate;
    int32_t cursor_x = g->current_cursor_focus->x_coordinate;

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
            new_layout[row + 1][column]->mark = g->display->layout[row][column]->mark;
            for (int cardinal_direction = NORTH; cardinal_direction < NUM_CARDINAL_DIRECTIONS; cardinal_direction++)
            {
                new_layout[row + 1][column]->exits[cardinal_direction] = g->display->layout[row][column]->exits[cardinal_direction];
            }
        }
    }

    // Transfer marked rooms:
    if (g->start)
        g->start = new_layout[g->start->y_coordinate + 1][g->start->x_coordinate];
    if (g->end)
        g->end = new_layout[g->end->y_coordinate + 1][g->end->x_coordinate];

    free_layout(g->display->layout, g->current_map->height);
    free_map(g->current_map);
    g->display->layout = new_layout;
    g->current_map = new_map;
    g->current_cursor_focus = g->display->layout[cursor_y + 1][cursor_x];

    // Resize display:
    if (g->display->height < g->current_map->height && g->current_map->height <= g->user_settings->max_display_height)
        g->display->height = g->current_map->height, g->display->y_offset = 0;

    return;
}

void add_column_east(Gamestate *g)
{
    // Store cursor position:
    int32_t cursor_y = g->current_cursor_focus->y_coordinate;
    int32_t cursor_x = g->current_cursor_focus->x_coordinate;

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
            new_layout[row][column]->mark = g->display->layout[row][column]->mark;
            for (int cardinal_direction = NORTH; cardinal_direction < NUM_CARDINAL_DIRECTIONS; cardinal_direction++)
            {
                new_layout[row][column]->exits[cardinal_direction] = g->display->layout[row][column]->exits[cardinal_direction];
            }
        }
    }

    // Transfer marked rooms:
    if (g->start)
        g->start = new_layout[g->start->y_coordinate][g->start->x_coordinate];
    if (g->end)
        g->end = new_layout[g->end->y_coordinate][g->end->x_coordinate];

    free_layout(g->display->layout, g->current_map->height);
    free_map(g->current_map);
    g->display->layout = new_layout;
    g->current_map = new_map;
    g->current_cursor_focus = g->display->layout[cursor_y][cursor_x];

    // Resize display:
    if (g->display->width < g->current_map->width && g->current_map->width <= g->user_settings->max_display_width)
        g->display->width = g->current_map->width, g->display->x_offset = 0;

    return;
}

void add_row_south(Gamestate *g)
{
    // Store cursor position:
    int32_t cursor_y = g->current_cursor_focus->y_coordinate;
    int32_t cursor_x = g->current_cursor_focus->x_coordinate;

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
            new_layout[row][column]->mark = g->display->layout[row][column]->mark;
            for (int cardinal_direction = NORTH; cardinal_direction < NUM_CARDINAL_DIRECTIONS; cardinal_direction++)
            {
                new_layout[row][column]->exits[cardinal_direction] = g->display->layout[row][column]->exits[cardinal_direction];
            }
        }
    }

    // Transfer marked rooms:
    if (g->start)
        g->start = new_layout[g->start->y_coordinate][g->start->x_coordinate];
    if (g->end)
        g->end = new_layout[g->end->y_coordinate][g->end->x_coordinate];

    free_layout(g->display->layout, g->current_map->height);
    free_map(g->current_map);
    g->display->layout = new_layout;
    g->current_map = new_map;
    g->current_cursor_focus = g->display->layout[cursor_y][cursor_x];

    // Resize display:
    if (g->display->height < g->current_map->height && g->current_map->height <= g->user_settings->max_display_height)
        g->display->height = g->current_map->height, g->display->y_offset = 0;

    return;
}

void add_column_west(Gamestate *g)
{
    // Store cursor position, adjusted inward if on eastern end of map (to prevent cursor moving out of display after column is added):
    int32_t cursor_y = g->current_cursor_focus->y_coordinate;
    int32_t cursor_x = g->current_cursor_focus->x_coordinate == (g->display->width - 1) + g->display->x_offset ? g->current_cursor_focus->x_coordinate - 1 : g->current_cursor_focus->x_coordinate;

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
            new_layout[row][column + 1]->mark = g->display->layout[row][column]->mark;
            for (int cardinal_direction = NORTH; cardinal_direction < NUM_CARDINAL_DIRECTIONS; cardinal_direction++)
            {
                new_layout[row][column + 1]->exits[cardinal_direction] = g->display->layout[row][column]->exits[cardinal_direction];
            }
        }
    }

    // Transfer marked rooms:
    if (g->start)
        g->start = new_layout[g->start->y_coordinate][g->start->x_coordinate + 1];
    if (g->end)
        g->end = new_layout[g->end->y_coordinate][g->end->x_coordinate + 1];

    free_layout(g->display->layout, g->current_map->height);
    free_map(g->current_map);
    g->display->layout = new_layout;
    g->current_map = new_map;
    g->current_cursor_focus = g->display->layout[cursor_y][cursor_x + 1];

    // Resize display:
    if (g->display->width < g->current_map->width && g->current_map->width <= g->user_settings->max_display_width)
        g->display->width = g->current_map->width, g->display->x_offset = 0;

    return;
}

void toggle_movement(Gamestate *g)
{
    if (g->user_settings->movement_mode == NESW)
        g->user_settings->movement_mode = WASD;
    else
        g->user_settings->movement_mode = NESW;
    return;
}

void mark(char mark, Gamestate *g)
{
    char yesno = 0;

    switch (mark)
    {
        default: error_code = 15; break;
        case 'S':
            if (g->start == g->current_cursor_focus)
            {
                // Do nothing.
            }
            else if (g->start != NULL)
            {
                do
                {
                    (void) printf("A different room has already been marked as the start. Would you like to erase that mark and place the start here instead? (y/n) ");
                    yesno = tolower(getchar()), gobble_line();
                } while (yesno != 'y' && yesno != 'n');
                if (yesno == 'y')
                {
                    g->start->mark = 0;
                    g->start = g->current_cursor_focus;
                    g->current_cursor_focus->mark = 'S';
                    if (g->end == g->start) // If overriding one mark with the other:
                    {
                        g->end = NULL;
                    }
                }
            }
            else
            {
                g->start = g->current_cursor_focus;
                g->current_cursor_focus->mark = 'S';
                if (g->end == g->start) // If overriding one mark with the other:
                {
                    g->end = NULL;
                }
            }
            break;
        case 'E':
            if (g->end == g->current_cursor_focus)
            {
                // Do nothing.
            }
            else if (g->end != NULL)
            {
                do
                {
                    (void) printf("A different room has already been marked as the end. Would you like to erase that mark and place the end here instead? (y/n) ");
                    yesno = tolower(getchar()), gobble_line();
                } while (yesno != 'y' && yesno != 'n');
                if (yesno == 'y')
                {
                    g->end->mark = 0;
                    g->end = g->current_cursor_focus;
                    g->current_cursor_focus->mark = 'E';
                    if (g->start == g->end) // If overriding one mark with the other:
                    {
                        g->start = NULL;
                    }
                }
            }
            else
            {
                g->end = g->current_cursor_focus;
                g->current_cursor_focus->mark = 'E';
                if (g->start == g->end) // If overriding one mark with the other:
                {
                    g->start = NULL;
                }
            }
            break;
        case 0:
            if (g->current_cursor_focus->mark != 0)
            {
                if (g->current_cursor_focus->mark == 'S')
                {
                    g->start = NULL;
                    g->current_cursor_focus->mark = 0;
                }
                else
                {
                    g->end = NULL;
                    g->current_cursor_focus->mark = 0;
                }
            }
            break;
    }
}

void delete(Gamestate *g)
{
    g->current_cursor_focus->exists = false;
    // Remove start/end mark so that it can be placed elsewhere (and so that undeleting later doesn't conflict with new start/end):
    mark(0, g);
    // Remove exits between this and surrounding rooms so the display doesn't end up with a hanging connection to a nonexistent room:
    for (int i = NORTH; i < NUM_CARDINAL_DIRECTIONS; i++)
    {
        g->current_cursor_focus->exits[i] = false;
    }
    if (g->current_cursor_focus->y_coordinate != 0)
        g->display->layout[g->current_cursor_focus->y_coordinate - 1][g->current_cursor_focus->x_coordinate]->exits[SOUTH] = false;
    if (g->current_cursor_focus->x_coordinate != g->current_map->width - 1)
        g->display->layout[g->current_cursor_focus->y_coordinate][g->current_cursor_focus->x_coordinate + 1]->exits[WEST] = false;
    if (g->current_cursor_focus->y_coordinate != g->current_map->height - 1)
        g->display->layout[g->current_cursor_focus->y_coordinate + 1][g->current_cursor_focus->x_coordinate]->exits[NORTH] = false;
    if (g->current_cursor_focus->x_coordinate != 0)
        g->display->layout[g->current_cursor_focus->y_coordinate][g->current_cursor_focus->x_coordinate - 1]->exits[EAST] = false;

    return;
}

void undelete(Gamestate *g)
{
    g->current_cursor_focus->exists = true;
    return;
}

void open(Gamestate *g, int direction)
{
    int yesno = 0;

    if (!g->current_cursor_focus->exists)
    {
        #ifdef FORCE_BUFFERED_MODE
            do
            {
                (void) printf("The current room has been deleted from the map. Would you like to restore it in order to add an opening? (y/n) ");
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
            undelete(g);
        }
        else
        {
            return;
        }
    }

    yesno = 0; //reset for next Qs

    switch (direction)
    {
        default: error_code = 16; break;
        case NORTH:
            if (g->current_cursor_focus->y_coordinate == 0)
            {
                #ifdef FORCE_BUFFERED_MODE
                    do
                    {
                        (void) printf("There is no room to the north. Would you like to shift the coordinate system and create a new row? (y/n) ");
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
                }
            }
            else if (!g->display->layout[g->current_cursor_focus->y_coordinate - 1][g->current_cursor_focus->x_coordinate]->exists)
            {
                #ifdef FORCE_BUFFERED_MODE
                    do
                    {
                        (void) printf("The room to the north has been deleted from the map. Would you like to restore it in order to add an opening? (y/n) ");
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
                    g->current_cursor_focus = g->display->layout[g->current_cursor_focus->y_coordinate - 1][g->current_cursor_focus->x_coordinate];
                    undelete(g);
                    g->current_cursor_focus = g->display->layout[g->current_cursor_focus->y_coordinate + 1][g->current_cursor_focus->x_coordinate];
                }
            }
            if (yesno == 'y' || yesno == 0)
            {
                g->current_cursor_focus->exits[NORTH] = true;
                g->display->layout[g->current_cursor_focus->y_coordinate - 1][g->current_cursor_focus->x_coordinate]->exits[SOUTH] = true;
            }
            break;
        case EAST:
            if (g->current_cursor_focus->x_coordinate == g->current_map->width - 1)
            {
                #ifdef FORCE_BUFFERED_MODE
                    do
                    {
                        (void) printf("There is no room to the east. Would you like to create a new column? (y/n) ");
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
                }
            }
            else if (!g->display->layout[g->current_cursor_focus->y_coordinate][g->current_cursor_focus->x_coordinate + 1]->exists)
            {
                #ifdef FORCE_BUFFERED_MODE
                    do
                    {
                        (void) printf("The room to the east has been deleted from the map. Would you like to restore it in order to add an opening? (y/n) ");
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
                    g->current_cursor_focus = g->display->layout[g->current_cursor_focus->y_coordinate][g->current_cursor_focus->x_coordinate + 1];
                    undelete(g);
                    g->current_cursor_focus = g->display->layout[g->current_cursor_focus->y_coordinate][g->current_cursor_focus->x_coordinate - 1];
                }
            }
            if (yesno == 'y' || yesno == 0)
            {
                g->current_cursor_focus->exits[EAST] = true;
                g->display->layout[g->current_cursor_focus->y_coordinate][g->current_cursor_focus->x_coordinate + 1]->exits[WEST] = true;
            }
            break;
        case SOUTH:
            if (g->current_cursor_focus->y_coordinate == g->current_map->height - 1)
            {
                #ifdef FORCE_BUFFERED_MODE
                    do
                    {
                        (void) printf("There is no room to the south. Would you like to create a new row? (y/n) ");
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
                }
            }
            else if (!g->display->layout[g->current_cursor_focus->y_coordinate + 1][g->current_cursor_focus->x_coordinate]->exists)
            {
                #ifdef FORCE_BUFFERED_MODE
                    do
                    {
                        (void) printf("The room to the south has been deleted from the map. Would you like to restore it in order to add an opening? (y/n) ");
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
                    g->current_cursor_focus = g->display->layout[g->current_cursor_focus->y_coordinate + 1][g->current_cursor_focus->x_coordinate];
                    undelete(g);
                    g->current_cursor_focus = g->display->layout[g->current_cursor_focus->y_coordinate - 1][g->current_cursor_focus->x_coordinate];
                }
            }
            if (yesno == 'y' || yesno == 0)
            {
                g->current_cursor_focus->exits[SOUTH] = true;
                g->display->layout[g->current_cursor_focus->y_coordinate + 1][g->current_cursor_focus->x_coordinate]->exits[NORTH] = true;
            }
            break;
        case WEST:
            if (g->current_cursor_focus->x_coordinate == 0)
            {
                #ifdef FORCE_BUFFERED_MODE
                    do
                    {
                        (void) printf("There is no room to the west. Would you like to shift the coordinate system and create a new column? (y/n) ");
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
                }
            }
            else if (!g->display->layout[g->current_cursor_focus->y_coordinate][g->current_cursor_focus->x_coordinate - 1]->exists)
            {
                #ifdef FORCE_BUFFERED_MODE
                    do
                    {
                        (void) printf("The room to the west has been deleted from the map. Would you like to restore it in order to add an opening? (y/n) ");
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
                    g->current_cursor_focus = g->display->layout[g->current_cursor_focus->y_coordinate][g->current_cursor_focus->x_coordinate - 1];
                    undelete(g);
                    g->current_cursor_focus = g->display->layout[g->current_cursor_focus->y_coordinate][g->current_cursor_focus->x_coordinate + 1];
                }
            }
            if (yesno == 'y' || yesno == 0)
            {
                g->current_cursor_focus->exits[WEST] = true;
                g->display->layout[g->current_cursor_focus->y_coordinate][g->current_cursor_focus->x_coordinate - 1]->exits[EAST] = true;
            }
            break;
    }
    return;
}

void close(Gamestate *g, int direction)
{
    if (g->current_cursor_focus->exists)
    {
        switch (direction)
        {
            default: error_code = 17; break;
            case NORTH:
                if (g->current_cursor_focus->y_coordinate != 0 && g->current_cursor_focus->exits[NORTH])
                {
                    g->current_cursor_focus->exits[NORTH] = false;
                    g->display->layout[g->current_cursor_focus->y_coordinate - 1][g->current_cursor_focus->x_coordinate]->exits[SOUTH] = false;
                }
                break;
            case EAST:
                if (g->current_cursor_focus->x_coordinate != g->current_map->width - 1 && g->current_cursor_focus->exits[EAST])
                {
                    g->current_cursor_focus->exits[EAST] = false;
                    g->display->layout[g->current_cursor_focus->y_coordinate][g->current_cursor_focus->x_coordinate + 1]->exits[WEST] = false;
                }
                break;
            case SOUTH:
                if (g->current_cursor_focus->y_coordinate != g->current_map->height - 1 && g->current_cursor_focus->exits[SOUTH])
                {
                    g->current_cursor_focus->exits[SOUTH] = false;
                    g->display->layout[g->current_cursor_focus->y_coordinate + 1][g->current_cursor_focus->x_coordinate]->exits[NORTH] = false;
                }
                break;
            case WEST:
                if (g->current_cursor_focus->x_coordinate != 0 && g->current_cursor_focus->exits[WEST])
                {
                    g->current_cursor_focus->exits[WEST] = false;
                    g->display->layout[g->current_cursor_focus->y_coordinate][g->current_cursor_focus->x_coordinate - 1]->exits[EAST] = false;
                }
                break;
        }
    }
    return;
}

void remove_row_north(Gamestate *g)
{
    // Store cursor position, adjusting inward if on row/column to delete:
    int32_t cursor_y = g->current_cursor_focus->y_coordinate == 0 ? g->current_cursor_focus->y_coordinate + 1 : g->current_cursor_focus->y_coordinate;
    int32_t cursor_x = g->current_cursor_focus->x_coordinate;

    Dimensions new_map_dim;
    new_map_dim.height = g->current_map->height - 1;
    new_map_dim.width = g->current_map->width;

    if (new_map_dim.height < 1)
    {
        (void) printf("Unable to comply: Map must have a minimum height of 1.\n");
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

    for (int32_t column = 0; column < g->current_map->width; column++)
    {
        g->current_cursor_focus = g->display->layout[0][column];
        delete(g);
    }

    // Transfer established Room properties:
    for (int32_t row = 0; row < g->current_map->height - 1; row++)
    {
        for (int32_t column = 0; column < g->current_map->width; column++)
        {
            new_layout[row][column]->exists = g->display->layout[row + 1][column]->exists;
            new_layout[row][column]->mark = g->display->layout[row + 1][column]->mark;
            for (int cardinal_direction = NORTH; cardinal_direction < NUM_CARDINAL_DIRECTIONS; cardinal_direction++)
            {
                new_layout[row][column]->exits[cardinal_direction] = g->display->layout[row + 1][column]->exits[cardinal_direction];
            }
        }
    }

    // Transfer marked rooms:
    if (g->start)
        g->start = new_layout[g->start->y_coordinate - 1][g->start->x_coordinate];
    if (g->end)
        g->end = new_layout[g->end->y_coordinate - 1][g->end->x_coordinate];

    free_layout(g->display->layout, g->current_map->height);
    free_map(g->current_map);
    g->display->layout = new_layout;
    g->current_map = new_map;
    g->current_cursor_focus = g->display->layout[cursor_y - 1][cursor_x];

    // Shrink offset:
    if (g->display->y_offset > 0)
        g->display->y_offset--;

    // Resize display:
    if (g->display->height > g->current_map->height)
        g->display->height = g->current_map->height, g->display->y_offset = 0;

    return;
}

void remove_column_east(Gamestate *g)
{
    // Store cursor position, adjusting inward if on row/column to delete:
    int32_t cursor_y = g->current_cursor_focus->y_coordinate;
    int32_t cursor_x = g->current_cursor_focus->x_coordinate == (g->display->width - 1) + g->display->x_offset ? g->current_cursor_focus->x_coordinate - 1 : g->current_cursor_focus->x_coordinate;

    Dimensions new_map_dim;
    new_map_dim.height = g->current_map->height;
    new_map_dim.width = g->current_map->width - 1;

    if (new_map_dim.width < 1)
    {
        (void) printf("Unable to comply: Map must have a minimum width of 1.\n");
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

    for (int32_t row = 0; row < g->current_map->height; row++)
    {
        g->current_cursor_focus = g->display->layout[row][g->current_map->width - 1];
        delete(g);
    }

    // Transfer established Room properties:
    for (int32_t row = 0; row < g->current_map->height; row++)
    {
        for (int32_t column = 0; column < g->current_map->width - 1; column++)
        {
            new_layout[row][column]->exists = g->display->layout[row][column]->exists;
            new_layout[row][column]->mark = g->display->layout[row][column]->mark;
            for (int cardinal_direction = NORTH; cardinal_direction < NUM_CARDINAL_DIRECTIONS; cardinal_direction++)
            {
                new_layout[row][column]->exits[cardinal_direction] = g->display->layout[row][column]->exits[cardinal_direction];
            }
        }
    }

    // Transfer marked rooms:
    if (g->start)
        g->start = new_layout[g->start->y_coordinate][g->start->x_coordinate];
    if (g->end)
        g->end = new_layout[g->end->y_coordinate][g->end->x_coordinate];

    free_layout(g->display->layout, g->current_map->height);
    free_map(g->current_map);
    g->display->layout = new_layout;
    g->current_map = new_map;
    g->current_cursor_focus = g->display->layout[cursor_y][cursor_x];

    // Shrink offset:
    if (g->display->x_offset > 0)
        g->display->x_offset--;

    // Resize display:
    if (g->display->width > g->current_map->width)
        g->display->width = g->current_map->width, g->display->x_offset = 0;

    return;
}

void remove_row_south(Gamestate *g)
{
    // Store cursor position, adjusting inward if on row/column to delete:
    int32_t cursor_y = g->current_cursor_focus->y_coordinate == (g->display->height - 1) + g->display->y_offset ? g->current_cursor_focus->y_coordinate - 1 : g->current_cursor_focus->y_coordinate;
    int32_t cursor_x = g->current_cursor_focus->x_coordinate;

    Dimensions new_map_dim;
    new_map_dim.height = g->current_map->height - 1;
    new_map_dim.width = g->current_map->width;

    if (new_map_dim.height < 1)
    {
        (void) printf("Unable to comply: Map must have a minimum height of 1.\n");
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

    for (int32_t column = 0; column < g->current_map->width; column++)
    {
        g->current_cursor_focus = g->display->layout[g->current_map->height - 1][column];
        delete(g);
    }

    // Transfer established Room properties:
    for (int32_t row = 0; row < g->current_map->height - 1; row++)
    {
        for (int32_t column = 0; column < g->current_map->width; column++)
        {
            new_layout[row][column]->exists = g->display->layout[row][column]->exists;
            new_layout[row][column]->mark = g->display->layout[row][column]->mark;
            for (int cardinal_direction = NORTH; cardinal_direction < NUM_CARDINAL_DIRECTIONS; cardinal_direction++)
            {
                new_layout[row][column]->exits[cardinal_direction] = g->display->layout[row][column]->exits[cardinal_direction];
            }
        }
    }

    // Transfer marked rooms:
    if (g->start)
        g->start = new_layout[g->start->y_coordinate][g->start->x_coordinate];
    if (g->end)
        g->end = new_layout[g->end->y_coordinate][g->end->x_coordinate];

    free_layout(g->display->layout, g->current_map->height);
    free_map(g->current_map);
    g->display->layout = new_layout;
    g->current_map = new_map;
    g->current_cursor_focus = g->display->layout[cursor_y][cursor_x];

    // Shrink offset:
    if (g->display->y_offset > 0)
        g->display->y_offset--;

    // Resize display:
    if (g->display->height > g->current_map->height)
        g->display->height = g->current_map->height, g->display->y_offset = 0;

    return;
}

void remove_column_west(Gamestate *g)
{
    // Store cursor position, adjusting inward if on row/column to delete:
    int32_t cursor_y = g->current_cursor_focus->y_coordinate;
    int32_t cursor_x = g->current_cursor_focus->x_coordinate ==  0 ? g->current_cursor_focus->x_coordinate + 1 : g->current_cursor_focus->x_coordinate;

    Dimensions new_map_dim;
    new_map_dim.height = g->current_map->height;
    new_map_dim.width = g->current_map->width - 1;

    if (new_map_dim.width < 1)
    {
        (void) printf("Unable to comply: Map must have a minimum width of 1.\n");
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

    for (int32_t row = 0; row < g->current_map->height; row++)
    {
        g->current_cursor_focus = g->display->layout[row][0];
        delete(g);
    }

    // Transfer established Room properties:
    for (int32_t row = 0; row < g->current_map->height; row++)
    {
        for (int32_t column = 0; column < g->current_map->width - 1; column++)
        {
            new_layout[row][column]->exists = g->display->layout[row][column + 1]->exists;
            new_layout[row][column]->mark = g->display->layout[row][column + 1]->mark;
            for (int cardinal_direction = NORTH; cardinal_direction < NUM_CARDINAL_DIRECTIONS; cardinal_direction++)
            {
                new_layout[row][column]->exits[cardinal_direction] = g->display->layout[row][column + 1]->exits[cardinal_direction];
            }
        }
    }

    // Transfer marked rooms:
    if (g->start)
        g->start = new_layout[g->start->y_coordinate][g->start->x_coordinate - 1];
    if (g->end)
        g->end = new_layout[g->end->y_coordinate][g->end->x_coordinate - 1];

    free_layout(g->display->layout, g->current_map->height);
    free_map(g->current_map);
    g->display->layout = new_layout;
    g->current_map = new_map;
    g->current_cursor_focus = g->display->layout[cursor_y][cursor_x - 1];

    // Shrink offset:
    if (g->display->x_offset > 0)
        g->display->x_offset--;

    // Resize display:
    if (g->display->width > g->current_map->width)
        g->display->width = g->current_map->width, g->display->x_offset = 0;

    return;
}

/*******************************************************************************************
 * save_gamestate:    Purpose: Saves the given gamestate to an external file in a bespoke file format; *
 *                       can be instructed to create new file or overwrite old file.       *
 *              Parameters: Gamestate *savable_gamestate -> the gamestate to be saved to file.               *
 *              Return value: none                                                         *
 *              Side effects: - Creates or edits external files.                           *
 *                            - Prints to stdout                                           *
 *                            - Reads from stdin                                           *
 *******************************************************************************************/
void save_gamestate(Gamestate *savable_gamestate)
{
    // TODO: Incorportate saving to new file and overriding old file.
    // TODO: Include ability to fetch list of files currently in system.
    // TODO: Similarly, or alternately, implement "Save" and "Save as..."

    int return_code = 0, y_n = 0;
    FILE *savefile = NULL;
    bool valid = false;
    int fclose_return = 0;

    if (savable_gamestate->current_filename != NULL)
    {
        (void) printf("Save file as %s? (y/n)\n", savable_gamestate->current_filename);
        do
        {
            y_n = tolower(getchar()); while (getchar() != '\n');
        } while (y_n != 'y' && y_n != 'n');
        if (y_n == 'n')
        {
            free(savable_gamestate->current_filename);
            savable_gamestate->current_filename = NULL;
        }
    }
    if (savable_gamestate->current_filename == NULL)
    {
        // Loop prompts for and stores valid filename to be used for the savefile:
        do
        {
            return_code = get_command("Save under what filename?\n> ", savable_gamestate, 's');
            if (return_code == -1)
                return;
            // Test whether a file with this name already exists:
            savefile = fopen(strcat(savable_gamestate->current_filename, ".ifmap"), "r");
            if (savefile == NULL)
                valid = true;
            else // If a file with this name already exists, confirm whether to save over it:
            {
                y_n = 0;
                (void) printf("A file with this filename already exists. Overwrite file? (y/n)\n");
                do
                {
                    y_n = tolower(getchar()); while (getchar() != '\n');
                } while (y_n != 'y' && y_n != 'n');
                if (y_n == 'y')
                    valid = true;
                fclose_return = fclose(savefile);
            }
        } while (!valid);
    }

    if (fclose_return == EOF)
    {
        error_code = 26;
        return;
    }

    savefile = fopen(strcat(savable_gamestate->current_filename, ".ifmap"), "w");

    int fwrite_return = 0;

    // From here to end: Encode data and write.

    // map height = int32_t
    // map width = int32_t
    // number of rooms (map height * map width) = int32_t
    int32_t buffer32n1[3] = {0};
    buffer32n1[0] = savable_gamestate->current_map->height;
    buffer32n1[1] = savable_gamestate->current_map->width;
    buffer32n1[2] = savable_gamestate->current_map->height * savable_gamestate->current_map->width; // Won't exceed int32 size due to MAX_COORDINATE.

    for (int i = 0; i < 3; i++)
    {
        fwrite_return = fwrite(buffer32n1[i], sizeof(int32_t), 1, savefile);
        if (fwrite_return != 1)
        {
            error_code = 27;
            fclose_return = fclose(savefile);
            if (fclose_return == EOF)
                error_code = 29;
            return;
        }
    }

    // loop:
    //      room y_coordinate = int32_t
    //      room x_coordinate = int32_t
    //      room exists = uint8_t
    //      room exit north = uint8_t
    //      room exit east = uint8_t
    //      room exit south = uint8_t
    //      room exit west = uint8_t
    //      room mark (0 for nothing, 1 for start, 2 for end) = uint8_t

    int32_t buffer32n2[2] = {0};
    uint8_t buffer8n1[6] = {0};
    Room *current_room = savable_gamestate->current_map->root;

    while (current_room != NULL)
    {
        buffer32n2[0] = current_room->y_coordinate;
        buffer32n2[1] = current_room->x_coordinate;

        buffer8n1[0] = current_room->exists ? 1 : 0;
        buffer8n1[1] = current_room->exits[NORTH] ? 1 : 0;
        buffer8n1[2] = current_room->exits[EAST] ? 1 : 0;
        buffer8n1[3] = current_room->exits[SOUTH] ? 1 : 0;
        buffer8n1[4] = current_room->exits[WEST] ? 1 : 0;
        buffer8n1[5] = current_room->mark == 0 ? 0 : current_room->mark == 'S' ? 1 : 2;
    
        for (int i = 0; i < 2; i++)
        {
            fwrite_return = fwrite(buffer32n2[i], sizeof(int32_t), 1, savefile);
            if (fwrite_return != 1)
            {
                error_code = 27;
                fclose_return = fclose(savefile);
                if (fclose_return == EOF)
                    error_code = 29;
                return;
            }
        }
        for (int i = 0; i < 6; i++)
        {
            fwrite_return = fwrite(buffer8n1[i], sizeof(uint8_t), 1, savefile);
            if (fwrite_return != 1)
            {
                error_code = 27;
                fclose_return = fclose(savefile);
                if (fclose_return == EOF)
                    error_code = 29;
                return;
            }
        }

        current_room = current_room->next_room;
    }

    // display height = int32_t
    // display width = int32_t
    // display y_offset = int32_t
    // display x_offset = int32_t
    int32_t buffer32n3[4] = {0};
    buffer32n3[0] = savable_gamestate->display->height;
    buffer32n3[1] = savable_gamestate->display->width;
    buffer32n3[2] = savable_gamestate->display->y_offset;
    buffer32n3[3] = savable_gamestate->display->x_offset;

    for (int i = 0; i < 4; i++)
    {
        fwrite_return = fwrite(buffer32n3[i], sizeof(int32_t), 1, savefile);
        if (fwrite_return != 1)
        {
            error_code = 27;
            fclose_return = fclose(savefile);
            if (fclose_return == EOF)
                error_code = 29;
            return;
        }
    }

    // settings movement mode = uint8_t
    uint8_t buffer8n2 = 0;
    buffer8n2 = savable_gamestate->user_settings->movement_mode == NESW ? 0 : 1;

    fwrite_return = fwrite(buffer8n2, sizeof(uint8_t), 1, savefile);
    if (fwrite_return != 1)
    {
        error_code = 27;
        fclose_return = fclose(savefile);
        if (fclose_return == EOF)
            error_code = 29;
        return;
    }

    // settings max display height = int32_t
    // settings max display width = int32_t
    // gamestate current_cursor_focus y_coordinate = int32_t
    // gamestate current_cursor_focus x_coordinate = int32_t
    int32_t buffer32n4[4] = {0};
    buffer32n4[0] = savable_gamestate->user_settings->max_display_height;
    buffer32n4[1] = savable_gamestate->user_settings->max_display_width;
    buffer32n4[2] = savable_gamestate->current_cursor_focus->y_coordinate;
    buffer32n4[3] = savable_gamestate->current_cursor_focus->x_coordinate;

    for (int i = 0; i < 4; i++)
    {
        fwrite_return = fwrite(buffer32n4[i], sizeof(int32_t), 1, savefile);
        if (fwrite_return != 1)
        {
            error_code = 27;
            fclose_return = fclose(savefile);
            if (fclose_return == EOF)
                error_code = 29;
            return;
        }
    }

    fclose_return = fclose(savefile);
    if (fclose_return == EOF)
        error_code = 28;
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

void free_gamestate(Gamestate *g)
{
    free(g->current_filename);
    free(g);

    return;
}

bool warn(Gamestate *g)
{
    for (;;)
    {
        // Display main menu:
        CLEAR_CONSOLE;
        (void) printf("WARNING! You have unsaved changes remaining. Are you sure you'd like to quit?\n");
        (void) printf("\nOptions:\n");
        (void) printf("1. Save and quit\n");
        (void) printf("2. Quit without saving\n");
        (void) printf("3. Save and continue\n");
        (void) printf("4. Continue without saving\n\n");

        // Prompt for and validate user input:
        int selection = 0;
        for (;;)
        {
            (void) printf("Enter option number:\n>");
            (void) scanf("%d", &selection), gobble_line();

            if (selection < 1 || selection > 4)
                (void) printf("Please pick from the available options.\n");
            else
                break;
        }

        // Parse user input and exit function:
        switch (selection)
        {
            case 1: return true;
            case 2: return false;
            case 3: return g->quit = false, true;
            case 4: return g->quit = false, false;
            default: return error_code = 24, false;
        }
    }
}

//FUTURE TODOS:
// - Make height/width of editor adjustable for different sized monitors. Maybe implement a settings menu with defaults?
// - Make height/width of editor a variable based on internal reading of terminal window size (including if user re-sizes terminal midway through execution).
// - Add ability to turn off and back on the axis labels.
// - Add ability to declare the edge of the map and make rooms connect to the beginning of a different map on the same level, to make it more feasable to build large spaces by dividing them into separate chunks each on their own map.
// - Combine the four add row/column functions into one, to reduce duplicate code.
// - Drastically reduce MAX_COORDINATE's size. No human would ever need a map that generous, and were one to be used, certain functions--like adding new rows/columns--would take a prohibitively long time (meaning DAYS of real time to create a duplicate map, for example).