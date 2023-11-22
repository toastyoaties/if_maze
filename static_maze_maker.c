/****************************************************************************************************
 * Name: static_maze_maker.c                                                                        *
 * File creation date: 2023-11-1                                                                    *
 * 1.0 date:                                                                                        *
 * Last modification date:                                                                          *
 * Author: Ryan Wells                                                                               *
 * Purpose: Tool for creating hand-made mazes for use in IF maze exploration program                *
 *          (preventing the need for coding each maze individually).                                *
 ****************************************************************************************************/

/* Preprocessing Directives (#include) */
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

/* Preprocessing Directives (#define) */
#define CLEAR_CONSOLE (void) printf("\033[H\033[2J\033[3J") // ANSI escapes for clearing screen and scrollback.
#define NUM_CARDINAL_DIRECTIONS 4
#define MAX_DISPLAY_HEIGHT 40
#define MAX_DISPLAY_WIDTH 25
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

typedef struct map
{
    int32_t height;
    int32_t width;
    Room *root; // Pointer to start of linked list containing all rooms
} Map;

typedef struct display
{
    long long **layout;
    int height;
    int width;
    int32_t y_offset;
    int32_t x_offset;
    long long cursor_id;
} Display;

/* Declarations of External Variables */
// none

/* Declarations of Global Variables */
int error_code = 0;

/* Prototypes for non-main functions */
Map *create_map(void);
Room *make_room(int32_t y_coordinate, int32_t x_coordinate, long long room_id);
Map *load_map(void);
Map *edit_map(Map *editable_map);
long long **create_initial_layout(Map *map_to_display);
void free_layout(long long **layout, int32_t height);
Display *initialize_display(long long **layout_array, int32_t array_height, int32_t array_width, Room *root);
void print_display(Display *display, Room *root);
char *ystr(int32_t y_coordinate);
int calculate_letter_digits(int32_t number_to_convert);
int32_t lower_boundary(int base, int power);
int calculate_letter_index(int32_t current_number, int current_digit, int32_t lower_boundary);
Room *find_room(Room *root, long long room_id);
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
            (void) scanf("%d", &selection); while (getchar() != '\n');

            if (selection < 1 || selection > 3)
                (void) printf("Please pick from the available options.\n");
            else
                break;
        }

        // Parse user input:
        switch (selection)
        {
            case 1: free_map(edit_map(create_map())); break;
            case 2: free_map(edit_map(load_map())); break;
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
    }
    return error_code;
}

/* Definitions of other functions */
/*****************************************************************************************
 * create_map:    Purpose: Creates blank map for further editing.                        *
 *                Parameters: none                                                       *
 *                Return value: Map * -> The created map, to be passed into editing.     *
 *                Side effects: - Clears screen and scrollback                           *
 *                              - Prints to stdout.                                      *
 *                              - Reads from stdin.                                      *
 *                              - Allocates memory.                                      *
 *****************************************************************************************/
Map *create_map(void)
{
    // Allocate memory for map:
    CLEAR_CONSOLE;
    (void) printf("Creating blank map...\n");
    Map *created_map = malloc(sizeof(Map));
    if (created_map == NULL)
    {
        error_code = 4;
        return NULL;
    }

    // Prompt for initial height:
    created_map->height = 0;
    for (;;)
    {
        (void) printf("Enter desired initial height of map: ");
        (void) scanf("%d", &(created_map->height)); while (getchar() != '\n');
        if (created_map->height < 1)
            (void) printf("Please enter an integer greater than zero.\n");
        else if (created_map->height > MAX_COORDINATE)
            (void) printf("Max height is %d.\n", MAX_COORDINATE);
        else
            break;
    }

    // Prompt for initial width:
    created_map->width = 0;
    for (;;)
    {
        (void) printf("Enter desired initial width of map: ");
        (void) scanf("%d", &(created_map->width)); while (getchar() != '\n');
        if (created_map->width < 1)
            (void) printf("Please enter an integer greater than zero.\n");
        else if (created_map->width > MAX_COORDINATE)
            (void) printf("Max width is %d.\n", MAX_COORDINATE);
        else
            break;
    }

    // Initialize linked list of rooms, starting from (0,0):
    created_map->root = NULL;
    long long next_id = 0;
    for (int32_t i = 0; i < created_map->height; i++)
    {
        for (int32_t j = 0; j < created_map->width; j++)
        {
            Room *r = make_room(i, j, next_id++);
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
    for (int i = 0; i < NUM_CARDINAL_DIRECTIONS; i++)
    {
        r->exits[i] = 0;
    }

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
Map *edit_map(Map *editable_map)
{
    CLEAR_CONSOLE;

    if (error_code) return editable_map;

    // Print grid with x & y axes numbered & lettered (like Battleship board)
    long long **layout = create_initial_layout(editable_map);
    if (error_code)
    {
        free_layout(layout, editable_map->height);
        return editable_map;
    }

    Display *display = initialize_display(layout, editable_map->height, editable_map->width, editable_map->root);
    if (error_code)
    {
        free_layout(layout, editable_map->height);
        return editable_map;
    }

    print_display(display, editable_map->root);
    if (error_code)
    {
        free_layout(layout, editable_map->height);
        free(display);
        return editable_map;
    }
    while (getchar() != '\n');

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
    return editable_map;
}

/**********************************************************************************************************
 * create_initial_layout:     Purpose: Allocates room for, and initializes,                               *
 *                                     a 2D array containing the ids of rooms to display                  *
 *                                     as visualized map during editing.                                  *
 *                            Parameters: Map *map_to_display -> the map data to create the array from.   *
 *                            Return value: long long ** -> a pointer to the array                        *
 *                            Side effects: - allocates memory                                            *
 **********************************************************************************************************/
long long **create_initial_layout(Map *map_to_display)
{
    //Create 2D grid to store room ids:
    int32_t ncols = map_to_display->width;
    int32_t nrows = map_to_display->height;

    long long **layout = malloc(sizeof(long long *) * nrows);
    if (layout == NULL)
    {
        error_code = 2;
        return NULL;
    }

    for (int32_t i = 0; i < nrows; i++)
    {
        layout[i] = malloc(sizeof(long long) * (ncols));
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
                    layout[y][x] = current->id;
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
 *                 Parameters: - long long **layout -> the layout to be freed                       *
 *                             - int32_t height -> the number of rows in the layout                 *
 *                 Return value: none                                                               *
 *                 Side effects: - Frees all memory associated with given layout. Cannot be undone. *
 ****************************************************************************************************/
void free_layout(long long **layout, int32_t height)
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
 *                            Parameters: long long **layout_array -> pointer to the 2D layout array to display *
 *                                        int32_t array_height -> the height of the 2D layout array             *
 *                                        int32_t array_width -> the width of the 2D layout array               *
 *                            Return value: Display * -> a pointer to the initialized Display                   *
 *                            Side effects: - allocates memory                                                  *
 ****************************************************************************************************************/
Display *initialize_display(long long **layout_array, int32_t array_height, int32_t array_width, Room *root)
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

    // Set d->cursor_id to the first extant room in the layout array (will always be at (0,0) in a fresh map):
    for (int y = 0; y < d->height; y++)
    {
        for (int x = 0; x < d->width; x++)
        {
            for (Room *current_room = root; current_room->next_room != NULL; current_room = current_room->next_room)
            {
                if (current_room->id == d->layout[y][x])
                {
                    if (current_room->exists)
                    {
                        d->cursor_id = current_room->id;
                        goto full_breakout;
                    }
                    else
                        break;
                }
            }
        }
    }
    full_breakout:

    return d;
}

/*****************************************************************************************
 * print_display:       Purpose: Accepts a given Display and prints it to the screen.    *
 *                      Parameters: Display *display -> the Display to be printed.       *
 *                      Return value: none                                               *
 *                      Side effects: - prints to stdout                                 *
 *                                    - frees memory allocated during printing process   *
 *****************************************************************************************/
void print_display(Display *display, Room *root)
{
    // typedef struct display
    // {
    //     long long **layout;
    //     int height;
    //     int width;
    //     int32_t y_offset;
    //     int32_t x_offset;
    //     long long cursor_id;
    // } Display;

    // Find max screen length of y-coordinates to display, for formatting purposes:
    char *longest_y_string = ystr(display->height - 1 + display->y_offset);
    if (error_code) return;
    int longest_letter_digits = strlen(longest_y_string);
    free(longest_y_string);

    // Find max screen length of x-coordinates to display, for formatting purposes:
    int longest_number_digits = snprintf(NULL, 0, "%d", display->width - 1 + display->x_offset);

    //Find printing width of one room + surrounding symbols:
    int min_cell_width = 5;
    int space_on_both_sides = 2;
    int cell_width = min_cell_width > longest_number_digits + space_on_both_sides ? min_cell_width : longest_number_digits + space_on_both_sides;
    int assumed_terminal_width_in_cols = MAX_DISPLAY_WIDTH * min_cell_width;
    if (assumed_terminal_width_in_cols < display->width * cell_width)
        display->width = assumed_terminal_width_in_cols / cell_width;
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
    for (int x = 0; x < display->width; x++)
    {
        // Create x-coordinate string:
        int needed_strlen = snprintf(NULL, 0, "%d", x + display->x_offset);
        char *x_str = malloc(sizeof(char) * (needed_strlen + 1));
        if (x_str == NULL)
        {
            error_code = 7;
            return;
        }
        (void) snprintf(x_str, sizeof(x_str), "%d", x + display->x_offset);
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

    for (int y = 0; y < display->height; y++)
    {
        // Row above room:
        // Print spaces where letter coordinates would go:
        for (int letter_digit = 0; letter_digit < longest_letter_digits; letter_digit++)
            (void) printf(" ");
        (void) printf(" ");

        // Print visible rooms:
        for (int x = 0; x < display->width; x++)
        {
            // Print spaces where left hyphens & left side of room would be on room line:
            for (int hyphen = 0; hyphen < left_hyphens + 1; hyphen++) // + 1 is for the left parenthesis of the room.
                (void) printf(" ");

            // Find pointer to room matching current coordinates:
            Room *current = find_room(root, display->layout[y][x]);
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
        char *str = ystr(y + display->y_offset);
        if (error_code) return;
        int letter_digits = strlen(str);
        if (longest_letter_digits > letter_digits)
            for (int letter_digit = 0; letter_digit < longest_letter_digits - letter_digits; letter_digit++)
                (void) printf(" ");
        (void) printf("%s ", str), free(str); // y-coordinates are displayed as letters for user QoL
        // Print visible rooms:
        for (int x = 0; x < display->width; x++)
        {
            // Find pointer to room matching current coordinates:
            Room *current = find_room(root, display->layout[y][x]);
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
            if (current->id == display->cursor_id)
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
        if (y == display->height - 1)
        {
            // Print spaces where letter coordinates would go:
            for (int letter_digit = 0; letter_digit < longest_letter_digits; letter_digit++)
                (void) printf(" ");
            (void) printf(" ");
            // Print visible rooms:
            for (int x = 0; x < display->width; x++)
            {
                // Print spaces where left hyphens & left side of room would be on room line:
                for (int hyphen = 0; hyphen < left_hyphens + 1; hyphen++) // + 1 is for the left parenthesis of the room.
                    (void) printf(" ");
                // Find pointer to room matching current coordinates:
                Room *current = find_room(root, display->layout[y][x]);
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
    for (int i = 0; i < letters_wide + 1; i++)
        str[i] = '\0';
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

/**********************************************************************************************
 * find_room():    Purpose: Finds the memory address of the Room matching the given id        *
 *                 Parameters: - Room *root -> the root node of the Rooms linked list         *
 *                             - long long room_id -> the id of the Room being searched for   *
 *                 Return value: Room * -> a pointer to the searched-for Room                 *
 *                 Side effects: none                                                         *
 **********************************************************************************************/
Room *find_room(Room *root, long long room_id)
{
    Room *current = root;
    while (current != NULL)
    {
        if (current->id == room_id)
            return current;
        current = current->next_room;
    }

    return NULL;
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