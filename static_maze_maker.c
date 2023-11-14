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

/* Preprocessing Directives (#define) */
#define CLEAR_CONSOLE (void) printf("\033[H\033[2J\033[3J") // ANSI escapes for clearing screen and scrollback.
#define NUM_CARDINAL_DIRECTIONS 4
#define MAX_DISPLAY_HEIGHT 40
#define MAX_DISPLAY_WIDTH 40
#define MAX_COORDINATE 321272405 // Because of the use of the pow() function, combined with the int32_t limit.
#define NUM_LETTERS 26
#define MAX_ID (321272405 * 321272405)
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
// none

/* Prototypes for non-main functions */
Map *create_map(void);
Room *make_room(int32_t y_coordinate, int32_t x_coordinate, long long room_id);
Map *load_map(void);
Map *edit_map(Map *editable_map);
long long **create_initial_layout(Map *map_to_display);
Display *initialize_display(long long **layout_array, int32_t array_height, int32_t array_width, Room *root);
void print_display(Display *display);
char *ystr(int32_t y_coordinate);
int calculate_letter_digits(int32_t number_to_convert);
int32_t lower_boundary(int base, int power);
int calculate_letter_index(int32_t current_number, int current_digit, int32_t lower_boundary);
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
    }
    // Exit program:
    quit:
    return 0;
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
                attach_point = r;
            }
        }
    }

    return created_map;
}

/*****************************************************************************************
 * make_room:    Purpose: Allocates memory for and initializes single room.              *
 *               Parameters: int32_t y_coordinate -> the y-coordinate to assign to the room  *
 *                           int32_t x_coordinate -> the x-coordinate to assign to the room  *
 *               Return value: Room * -> a pointer to the new room                       *
 *               Side effects: - Allocates memory.                                       *
 *****************************************************************************************/
Room *make_room(int32_t y_coordinate, int32_t x_coordinate, long long room_id)
{
    Room *r = malloc(sizeof(Room));
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

    //TODO

    // Print grid with x & y axes numbered & lettered (like Battleship board)
    long long (*layout)[editable_map->width] = create_initial_layout(editable_map);

    Display *display = initialize_display(layout, editable_map->height, editable_map->width, editable_map->root);

    print_display(&display);

    // Prompt for cursor start location (represented by *)
    //  Alternately, start cursor on top line, furthest room to left.
    // Allow commands for moving cursor cardinally ("move up")
    // Allow commands for moving cursor cardinally while skipping over non-existent rooms ("skip up")
    // Allow commands for cursor jumping ("jump to A3")
    // Allow commands for establishing connecting doors between rooms (cardinally)
    // Allow commands for removing connections
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
    // Allow commands for saving maze to file and reading maze from file
    // Allow commands for translating entire map plus or minus x x-values or y y-values
    // Allow commands to establish critical path (in separate ANSI color)
    // Allow command to highlight which rooms cannot be reached
    // Allow command to determine whether critical path exists


    //TODO: Allow saving map before returning (returning leads to freeing--aka losing--map from memory)
    //TODO: Warns when about to return without saving
    free(layout);
    free(display);
    return editable_map;
}

/**********************************************************************************************************
 * create_initial_layout:     Purpose: Allocates room for, and initializes,                               *
 *                                     a 2D array containing the ids of rooms to display                  *
 *                                     as visualized map during editing.                                  *
 *                            Parameters: Map *map_to_display -> the map data to create the array from.   *
 *                            Return value: long long ** -> a pointer to the array                              *
 *                            Side effects: - allocates memory                                            *
 **********************************************************************************************************/
long long **create_initial_layout(Map *map_to_display)
{
    //Create 2D grid to store room ids:
    long long (*layout)[map_to_display->width]; //<-represents a single row in the form of a pointer to an array of <width> ints
    layout = malloc(sizeof(*layout) * map_to_display->height); // Allocating memory for <height> number of rows

    for (int32_t y = 0; y < map_to_display->height; y++)
    {
        for (int32_t x = 0; x < map_to_display->width; x++)
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

/**********************************************************************************************************
 * initialize_display:        Purpose: Allocates room for, and initializes, a Display.                    *
 *                            Parameters: long long **layout_array -> pointer to the 2D layout array to display *
 *                                        int32_t array_height -> the height of the 2D layout array           *
 *                                        int32_t array_width -> the width of the 2D layout array             *
 *                            Return value: Display * -> a pointer to the initialized Display             *
 *                            Side effects: - allocates memory                                            *
 **********************************************************************************************************/
Display *initialize_display(long long **layout_array, int32_t array_height, int32_t array_width, Room *root)
{
    Display *d = malloc(sizeof(Display));

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
 *****************************************************************************************/
void print_display(Display *display)
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

    for (int y = 0; y < display->height; y++)
    {
        for (int x = 0; x < display->width; x++)
        {
            char *str = ystr(y + display->y_offset);
            (void) printf("%s", str), free(str); // y-coordinates are displayed as letters for user QoL
        }
    }

    return;
}

/********************************************************************************************
 * ystr:    Purpose: Converts a given number coordinate into a letter coordinate.           *
 *          Parameters: int32_t y_coordinate -> the number coordinate to be converted           *
 *          Return value: char * -> a pointer to a string containing the letter coordinate  *
 *          Side effects: - allocates memory                                                *
 ********************************************************************************************/
char *ystr(int32_t y_coordinate)
{
    int letters_wide = calculate_letter_digits(y_coordinate);
    char *str = malloc(sizeof(char) * (letters_wide + 1));
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

// calculates the number of letter-digits (eg, A is 1 digit, ZGM is 3 digits) the conversion of the given numerical input will result in.
int calculate_letter_digits(int32_t number_to_convert)
{
    int digits = 1;
    while (number_to_convert >= lower_boundary(NUM_LETTERS, digits))
        digits++;
    return digits;
}

// calculates the lower boundary of the coordinate range created by the current exponentiation iteration
int32_t lower_boundary(int base, int power)
{
    int32_t sum = 0;
    while (power > 0)
        sum += pow(base, power--);
    return sum;
}

// calculates the index (within the alphabet, A-Z : 0-25) of the current digit in the letter coordinate
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
// - Add ability to turn off and back on the axis labels.
// - Add ability to declare the edge of the map and make rooms connect to the beginning of a different map on the same level, to make it more feasable to build large spaces by dividing them into separate chunks each on their own map.