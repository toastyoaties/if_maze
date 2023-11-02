/****************************************************************************************************
 * Name: static_maze_maker.c                                                                        *
 * File creation date: 2023-11-1		                                                            *
 * 1.0 date:		                                                                                *
 * Last modification date:	                                                                        *
 * Author: Ryan Wells                                                                               *
 * Purpose: Tool for creating hand-made mazes for use in IF maze exploration program                *
 *          (preventing the need for coding each maze individually).                                *
 ****************************************************************************************************/

/* Preprocessing Directives (#include) */
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

/* Preprocessing Directives (#define) */
#define CLEAR_CONSOLE (void) printf("\033[H\033[2J\033[3J") // ANSI escapes for clearing screen and scrollback.

/* Type Definitions */
enum cardinal_directions {
    NORTH,
    EAST,
    SOUTH,
    WEST,
};

typedef struct room
{
    int id;
    char *id_alias;
    int y_coordinate;
    int x_coordinate;
    bool exists;
    bool exits[4];
    struct room *next_room;
} Room;

typedef struct map
{
    int height;
    int width;
    Room *root; // Pointer to start of linked list containing all rooms
} Map;

/* Declarations of External Variables */
// none

/* Prototypes for non-main functions */
Map *create_map(void);
Room *initialize_rooms(int rooms_to_create);
Map *load_map(void);
Map *edit_map(Map *editable_map);
void save_map(Map *savable_map);
void free_map(Map *freeable_map);
void free_rooms(Room *r);

/* Definition of main */
/*****************************************************************************************
 * main:                Purpose: Runs main menu loop.                                    *
 *                      Parameters: none                                                 *
 *                      Return value: int                                                *
 *                      Side effects: - Prints to stdout                                 *
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
            (void) printf("Enter option number: ");
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
 *                Side effects: - Prints to stdout.                                      *
 *                              - Reads from stdin.                                      *
 *                              - Allocates memory.                                      *
 *****************************************************************************************/
Map *create_map(void)
{
    //TODO

    // Prompt for desired width & height of starter grid

    /*
    A Map contains:
        int height;
        int width;
        Room *root; // Pointer to start of linked list containing all rooms
    */

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
        else
            break;
    }

    created_map->root = initialize_rooms(created_map->height * created_map->width, height, width);

    return created_map;
}

/*****************************************************************************************
 * name_of_function:    Purpose:                                                         *
 *                      Parameters (and the meaning of each):                            *
 *                      Return value:                                                    *
 *                      Side effects (such as modifying external variables,              *
 *                          printing to stdout, or exiting the program):                 *
 *****************************************************************************************/
Room *initialize_rooms(int rooms_to_create)
{
    /*
    typedef struct room
    {
        int id;
        char *id_alias;
        int y_coordinate;
        int x_coordinate;
        bool exists;
        bool exits[4];
        struct room *next_room;
    } Room;
    */
    // TODO: Recursively(?) initialize rooms
    // # of rooms to initialize is represented by height * width
    Room *root = malloc(sizeof(Room));

    if 

    return root;
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
    Map *loaded_map = NULL;
    return loaded_map;
}

/********************************************************************************************
 * edit_map:    Purpose: The bulk of the program. Allows the user to edit the passed map.   *
 *              Parameters: Map *editable_map -> the map to be edited.                      *
 *              Return value: Map * -> the now-edited map,                                  *
 *                                     to be freed before further program operation         *
 *                                     or program termination.                              *
 *              Side effects: - Calls save_map, which edits external files.                 *
 *                            - Prints to stdout.                                           *
 *                            - Reads from stdin.                                           *
 *                            - Modifies any and all data associated with passed map.       *
 ********************************************************************************************/
Map *edit_map(Map *editable_map)
{
    //TODO

    // Print grid with x & y axes numbered & lettered (like Battleship board)
    // Prompt for cursor start location (represented by *)
    //  Alternately, start cursor on top line, furthest room to left.
    // Allow commands for moving cursor cardinally ("move up")
    // Allow commands for moving cursor cardinally while skipping over non-existent rooms ("skip up")
    // Allow commands for cursor jumping ("jump to A3")
    // Allow commands for establishing connecting doors between rooms (cardinally)
    // Allow commands for removing connections
    // Allow commands for expanding grid by x rows or columns
    // Allow commands for expanding grid by adding new individual rooms
    // Allow commands for deleting individual rooms
    // Allow commands for retracting grid by subtracting x rows or columns
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
    return editable_map;
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