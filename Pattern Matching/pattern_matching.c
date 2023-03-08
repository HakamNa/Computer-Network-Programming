#include "pattern_matching.h"

#include <stdlib.h>

#include <stdio.h>

#include <string.h>

#include "slist.h"

// ****************************** Private functions ******************************
int initialize_new_state(pm_t *fsm, pm_state_t *new_state);

int pm_bfs(pm_t *fsm, int *point_to_start, int *point_to_end, pm_state_t **queue);

void store_matches(pm_state_t *state, int i, dbllist_t *match_pattern_list);

void delete(pm_state_t *state);
// ****************************** Exercise functions ******************************

//Initializes the fsm parameters
int pm_init(pm_t *fsm) {
    // Allocate memory for the zero state and its output and transitions lists
    fsm->zerostate = (pm_state_t *) malloc(sizeof(pm_state_t));
    fsm->zerostate->output = (dbllist_t *) malloc(sizeof(dbllist_t));
    fsm->zerostate->_transitions = (dbllist_t *) malloc(sizeof(dbllist_init));
    // Return error if any of the allocations failed
    if (fsm == NULL || fsm->zerostate == NULL || fsm->zerostate->output == NULL ||
        fsm->zerostate->_transitions == NULL) {
        return -1;
    }
    // Initialize the values of the zero state
    fsm->zerostate->depth = 0;
    fsm->zerostate->id = 0;
    dbllist_init(fsm->zerostate->output);
    dbllist_init(fsm->zerostate->_transitions);
    fsm->newstate = 1; // The zero state is the first state to be allocated
    fsm->zerostate->fail = NULL;
    return 0;
}

// Set a transition arrow from this from_state, via a symbol, to a to_state.
int pm_goto_set(pm_state_t *from_state, unsigned char symbol, pm_state_t *to_state) {
    // Allocate memory for the labeled edge representing the transition from the from_state to the to_state with symbol.
    struct pm_labeled_edge *label = (pm_labeled_edge_t *) malloc(sizeof(pm_labeled_edge_t));
    // Check if memory allocation for the labeled edge was successful and if both the from_state and to_state are valid.
    if (label == NULL || from_state->_transitions == NULL || from_state == NULL || to_state == NULL) {
        return -1;
    }
    // Print the transition in the form "from_state_id -> symbol -> to_state_id".
    printf("%d -> %c -> %d\n", from_state->id, symbol, to_state->id);
    label->label = symbol;
    label->state = to_state;
    // Add the labeled edge representing the transition to the list of transitions for the from_state.
    if (dbllist_append(from_state->_transitions, label) == -1)
        return -1;
    return 0;
}

//  Returns the transition state.
pm_state_t *pm_goto_get(pm_state_t *state, unsigned char symbol) {
    // If the state or its transition list is not allocated, return NULL.
    if (state == NULL || state->_transitions == NULL) {
        return NULL;
    } else {
        // Iterate over the transitions in the state's transition list.
        struct dbllist_node *temp_node;
        for (temp_node = dbllist_head(state->_transitions); temp_node != NULL; temp_node = dbllist_next(temp_node)) {
            // If the symbol of the transition matches the given symbol, return the state it leads to.
            if (((pm_labeled_edge_t *) dbllist_data(temp_node))->label == symbol) {
                return ((pm_labeled_edge_t *) dbllist_data(temp_node))->state;
            }
        }
        // If no matching transition was found, return NULL.
        return NULL;
    }
}

// Adds a new string to the fsm, given that the string is of length n.
int pm_addstring(pm_t *fsm, unsigned char *str, _size_t n) {
    // Declare variables to store the current state, the next state, and a new state to be added
    struct pm_state *from_state, *to_state, *adding_state;
    // Set the current state to the zero state
    from_state = fsm->zerostate;
    // Check if the fsm, string, and zero state are all valid
    if (fsm == NULL || str == NULL || from_state == NULL) {
        return -1;
    }
    // Initialize a loop variable to zero
    int i = 0;
    // Iterate over the string
    while (i < n) {
        // Get the next state by following a transition from the current state with the current character in the string
        to_state = pm_goto_get(from_state, str[i]);
        // If the next state does not exist, create a new state
        if (to_state == NULL) {
            // Allocate memory for the new state
            adding_state = (pm_state_t *) malloc(sizeof(pm_state_t));
            // If memory allocation fails, return an error
            if (adding_state == NULL)
                return -1;
            // Initialize the new state
            if (initialize_new_state(fsm, adding_state) == -1)
                return -1;
            // Add a transition from the current state to the new state with the current character in the string
            if (pm_goto_set(from_state, str[i], adding_state) == -1) {
                return -1;
            }
            // Update the current state to the new state
            from_state = adding_state;

        } else {
            // If the next state already exists, simply update the current state to the next state
            from_state = to_state;
        }
        // Increment the loop variable to move on to the next character in the string
        i++;
    }
    // Add the string to the output list of the current state
    dbllist_append(from_state->output, str);
    // Return success
    return 0;
}

// Finalizes construction by setting up the failrue transitions, as well as the goto transitions of the zerostate.
int pm_makeFSM(pm_t *fsm) {
    // Allocate memory for pointers used in the function
    int *point_to_start = malloc(sizeof(int));
    int *point_to_end = malloc(sizeof(int));
    struct pm_state **queue = malloc(fsm->newstate * sizeof(pm_state_t *));
    if (fsm == NULL || queue == NULL) {
        // Either fsm or queue is NULL, return -1
        return -1;
    }
    // Initialize the pointers
    *point_to_start = 0;
    *point_to_end = 1;
    queue[0] = fsm->zerostate;

    // Call the pm_bfs function to perform a BFS on the FSM
    int bfs_result = pm_bfs(fsm, point_to_start, point_to_end, queue);
    if (bfs_result == -1) {
        // The BFS failed, return -1
        return -1;
    }

    // Free the queue and pointers
    free(queue);
    free(point_to_start);
    free(point_to_end);
    return 0;
}

// Search for matches in a string of size n in the FSM.
dbllist_t *pm_fsm_search(pm_state_t *state, unsigned char *str, _size_t n) {
    // Initialize the list to store the matched patterns
    struct dbllist *match_pattern_list = (dbllist_t *) malloc(sizeof(dbllist_t));
    if (state == NULL || match_pattern_list == NULL)
        return NULL;
    else
        dbllist_init(match_pattern_list);
    if (match_pattern_list == NULL)
        return NULL;

    // Start the search from the given state
    pm_state_t *current_state = state;

    // Iterate through the input string
    for (int i = 0; i < n; i++) {
        // If there is no transition from the current state for the current symbol, move to the fail state
        while (pm_goto_get(current_state, str[i]) == NULL) {
            // If we are at the end of the string, break the loop
            if ((i + 1) != n) {
                // If we are at the zero state, skip to the next symbol
                if (current_state->id == 0)
                    i += 1;
                else
                    // Otherwise, move to the fail state
                    current_state = current_state->fail;
            } else
                break;
        }
        // If there is a transition from the current state for the current symbol, move to the next state
        if (pm_goto_get(current_state, str[i]) != NULL)
            current_state = pm_goto_get(current_state, str[i]);

        // If there are any patterns in the output list of the current state, store them in the match list
        if (dbllist_head(current_state->output) != NULL) {
            store_matches(current_state, i, match_pattern_list);
            // If the current state has no transitions, move to the fail state
            if (dbllist_head(current_state->_transitions) == NULL)
                current_state = current_state->fail;
        }
    }
    return match_pattern_list;
}

// Destroys the fsm, deallocating memory.
void pm_destroy(pm_t *pm) {
    if (pm == NULL) {
        return;
    }
    // Deallocate the zero state and all of its contents
    free(pm->zerostate);
}

// ****************************** Private functions structure ******************************

/* This function initializes a new state in the finite state machine (FSM).
 * The main work of this function is to allocate memory for the new state, assign an ID and
 * depth to it, and update the newstate counter of the FSM. If the memory
 * allocation fails, the function returns -1
 */
int initialize_new_state(pm_t *fsm, pm_state_t *new_state) {
    // Print the ID of the new state that will be allocated
    printf("Allocating state %d\n", fsm->newstate);
    // Increment the depth of the new state
    new_state->depth++;
    // Assign the ID of the new state
    new_state->id = fsm->newstate;
    // Set the fail pointer of the new state to NULL
    new_state->fail = NULL;
    // Allocate memory for the output list of the new state
    new_state->output = (dbllist_t *) malloc(sizeof(dbllist_t));
    // Allocate memory for the transition list of the new state
    new_state->_transitions = (dbllist_t *) malloc(sizeof(dbllist_init));
    // If memory allocation fails, deallocate the transition list and return -1
    if (new_state->output == NULL || new_state->_transitions == NULL) {
        dbllist_destroy(new_state->_transitions, 0);
        free(new_state->_transitions);
        return -1;
    }
    // Initialize the output and transition lists of the new state
    dbllist_init(new_state->output);
    dbllist_init(new_state->_transitions);
    // Increment the counter of new states in the FSM
    fsm->newstate++;
    return 0;
}

/*
 *  performing a breadth-first search (BFS) on a finite state machine (FSM)
 *  and set the fail transitions for each state in the FSM.
 *  The fail transitions are used to quickly skip over substrings that are not a prefix
 *  of any patterns in the FSM when searching for patterns in a given text
 */
int pm_bfs(pm_t *fsm, int *point_to_start, int *point_to_end, pm_state_t **queue) {
    // Perform a BFS on the FSM
    while (*point_to_start < *point_to_end) {
        // Get the current state from the queue
        pm_state_t *current_state = queue[(*point_to_start)++];
        // Iterate through all possible characters
        for (unsigned int current_char = 0; current_char < 256; current_char++) {
            // Get the next state in the FSM by following the transition from the current state with the current character
            pm_state_t *next_state = pm_goto_get(current_state, current_char);
            if (next_state) {
                // Add the next_state to the queue
                queue[(*point_to_end)++] = next_state;
                pm_state_t *fail_state = current_state->fail;
                // Find the longest proper suffix of the current state that is a prefix of some pattern
                while (fail_state && !pm_goto_get(fail_state, current_char)) {
                    fail_state = fail_state->fail;
                }
                // Set the fail transition for the next_state
                if (!fail_state) {
                    next_state->fail = fsm->zerostate;
                } else {
                    next_state->fail = pm_goto_get(fail_state, current_char);
                    printf("Setting f(%d) = %d\n", next_state->id, next_state->fail->id);
                }
                // Check if either the output list of the next state or the output list of its fail state is NULL
                if (next_state->output == NULL || next_state->fail->output == NULL) {
                    return -1;
                }
                // Concat the output lists of the next_state and its fail state
                if (dbllist_tail(next_state->output) != NULL) {
                    dbllist_next(dbllist_tail(next_state->output)) = dbllist_head(next_state->fail->output);
                } else {
                    dbllist_head(next_state->output) = dbllist_head(next_state->fail->output);
                }
                dbllist_tail(next_state->output) = dbllist_tail(next_state->fail->output);
            }
        }
    }
    // The BFS was successful, return 0
    return 0;
}

/*
Initialize a new state in the finite state machine (fsm).
This includes allocating memory for the state and its output and transition lists,
as well as setting the state's ID and depth in the fsm.
*/
void store_matches(pm_state_t *state, int i, dbllist_t *match_pattern_list) {
    // Iterate over the output list of the state
    dbllist_node_t *temp = dbllist_head(state->output);
    while (temp != NULL) {
        // Create a new match object
        pm_match_t *match = (pm_match_t *) malloc(sizeof(pm_match_t));
        if (match == NULL)
            return;
        // Store the pattern, start and end positions, and the final state in the match object
        match->pattern = dbllist_data(temp);
        match->start_pos = i - (strlen(dbllist_data(temp)) - 1);
        match->end_pos = i;
        match->fstate = state;
        // Append the match object to the match_pattern_list
        if (dbllist_append(match_pattern_list, match) == -1)
            return;
        // Move to the next pattern in the output list
        temp = dbllist_next(temp);
    }

}

/*
 * deallocate memory for a given pm_state_t struct and all of its contents,
 * including its output list, transitions list, and fail state.
 * It does this recursively, starting with the fail state, then the transitions list,
 * the output list, and finally the pm_state_t struct itself.
 */
void delete(pm_state_t *state) {
    if (state == NULL) {
        return;
    }
    // Deallocate the output list and all of its contents
    dbllist_destroy(state->output, DBLLIST_FREE_DATA);
    // Deallocate the transitions list and all of its contents
    dbllist_destroy(state->_transitions, DBLLIST_FREE_DATA);
    // Recursively deallocate the fail state and all of its contents
    delete(state->fail);
    // Deallocate the pm_state_t struct itself
    free(state);
}

