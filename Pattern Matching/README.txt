Authored informations:

	Name: hakam nabulssi
  Ex.No: 1


file information:

	Exercise name: String Pattern Matching
	Submit files: readme, pattern_matching.c and slist.c


Folder content:

•slist.c: is a double linked list implementation, with functions for initializing, destroying, appending, prepending, and removing nodes from the list.
    The slist class uses macro definitions to access the fields of the list and node structures, which can improve code readability and maintainability
    by allowing the programmer to use more descriptive names for these fields.
    The list structure contains fields for the head, tail, and size of the list, while the node structure contains fields for the data stored in the node
    and pointers to the previous and next nodes.

•Pattern_matching.c: the pattern_matching is an implementation of a finite state machine (fsm),
    with functions for initializing the fsm, setting and getting transition states,adding strings to the fsm,
    searching the fsm for a match with  a given string, and destroying the fsm.The pattern_matching class includes
    a structure for representing a state in the fsm, which contains fields for the state's id, depth in the fsm,
    and pointers to the state's fail state and output list, as well as a list of transitions  from the state.
    It also includes a structure for representing a labeled edge, which consists of a label (the character that the edge represents) and a state.
    The main fsm structure includes fields for the zero state, the number of new states, and a list of all added strings.

•Readme.txt: text file that contains information about a project, including its purpose, how to build and run it, and any dependencies it has,
    to helps users understand what the project is about, how to build and run it, and what they can do with it.
    It also helps contributors understand the project's structure.


Main functions of slist.c file:

dbllist_init:
•	list: a pointer to the list to be initialized

    this function initializes the fields of the given list to their default values, setting the head and tail nodes to null and the size to 0.
    It is typically called when creating a new list to ensure that it is in a known state.

dbllist_destroy:
•	list: a pointer to the list to be destroyed
•	dealloc: a flag indicating whether to free the data stored in the list's nodes

    this function frees the memory used by the given list and its nodes. If the dealloc flag is set to dbllist_free_data,
    it also frees the data stored in each node. This  function should be called when the list is no longer needed to
    release the memory used by the list and avoid memory leaks.

dbllist_append:
•	list: a pointer to the list to which an element is to be added
•	data: a pointer to the data to be added to the list

    this function appends a new node containing the given data to the end of the given list.
    It returns 0 on success and -1 on failure (e. G. If the list or the new node cannot be allocated).

dbllist_prepend:
•	list: a pointer to the list to which an element is to be added
•	data: a pointer to the data to be added to the list

    this function adds a new node containing the given data to the beginning of the given list.
    It returns 0 on success and -1 on failure (e. G. If the list or the new node cannot be allocated).

dbllist_remove:
•	list: a pointer to the list from which a node is to be removed
•	node: a pointer to the node to be removed from the list
•	dealloc: a flag indicating whether to free the data stored in the node

    this function removes the given node from the given list. If the dealloc flag is set to dbllist_free_data,
    it also frees the data stored in the node. This function returns 0 on success and -1 if the node is not in the list.


Main functions of pettern_matching.c file:

pm_init:
•	fsm: a pointer to the fsm to be initialized

    this function initializes the given fsm. It allocates memory for the zero state, the list of
    transitions and output, and initializes the values for these structures. It returns 0 on success and -1 if malloc fails.

Pm_goto_set:
•	from_state: a pointer to the state from which the transition is to be made
•	symbol: the symbol to be used for the transition
•	to_state: a pointer to the state to which the transition is to be made

    this function sets a transition arrow from the from_state, via the given symbol, to the to_state.
    It returns 0 on success and -1 if malloc fails or if any of the given states are null.

Pm_goto_get:
•	state: a pointer to the state from which the transition is to be made
•	symbol: the symbol to be used for the transition

    this function returns a pointer to the state reached by transitioning from the given state, using the given symbol.
    It returns null if the given state or its list of transitions is null, or if no such transition exists.

Pm_addstring:
•	fsm: a pointer to the fsm to which the string is to be added
•	string: a pointer to the string to be added
•	n: the length of the string

    output: the output value to be associated with the string

    this function adds the given string to the given fsm, and associates it with the given output value.
    It returns 0 on success and -1 if any of the given pointers are null or if the function pm_goto_set fails.

Pm_makeFSM:
•	fsm: fsm: a pointer to a structure of type pm_t, representing a finite state machine.

    output: an integer indicating the result of the function. Returns 0 if the function was successful, and -1 if there was an error.

    This function creates a finite state machine (FSM) from a given structure of type pm_t.
    It does this by performing a breadth-first search (BFS) on the FSM and initializing the necessary pointers and data structures.
    The function returns 0 if the FSM was  successfully created, and -1 if there was an error.

Pm_fsm_search:
•	fsm: a pointer to the fsm to be used for matching
•	string: a pointer to the string to be matched
•	n: the length of the string

    outputs: a pointer to a list in which the output values for the matched strings will be stored

    this function searches for matches of the given string in the given fsm, and stores the output values for these matches
    in the given list. It returns 0 on success and -1 if any of the given pointers are null or if the function dbllist_append fails.

Pm_destroy:
•	fsm: a pointer to the fsm to be destroyed

    this function frees the memory allocated for the given fsm and its states and lists.
    It takes the fsm as input and does not return any value.


Private functions in slist.c:
    There are no private functions in this file


Private functions in patter_matching.c:

1.	int initialize_new_state (pm_t *fsm, pm_state_t *new_state):

•	(pm_t *fsm) - a pointer to a pm_t data structure
•	(pm_state_t *new_state) - a pointer to a pm_state_t data structure

    the job of this function is to update the information of a new state in the fsm, such as its depth, id, and initializing
    its output and transitions list. It also increments the counter of the new states in the fsm.
    The function returns 0 if the update was successful, and -1 if it failed.

2.	int pm_bfs(pm_t *fsm, int *point_to_start, int *point_to_end, pm_state_t **queue):

•	(pm_t *fsm) - a pointer to a structure representing a finite state machine (FSM).
•	(int *point_to_start) - a pointer to an integer representing the current position in the queue of states in the BFS.
•	(int *point_to_end) - a pointer to an integer representing the end position in the queue of states in the BFS.
•	(pm_state_t **queue) - a pointer to an array of pointers to pm_state_t structures representing the queue of states in the BFS.

    The function performs a breadth-first search (BFS) on the finite state machine (FSM) represented by the structure
    pointed to by the fsm parameter. It does so by iterating through all possible characters, following the transitions
    from the current state in the FSM, and adding the next state to the queue of states if it exists. It also sets the fail
    transition for the next state by finding the longest proper suffix of the current state that is a prefix of some pattern,
    and concatenates the output lists of the next state and its fail state.
    The function returns 0 if the BFS was successful, and -1 if either the output list
    of the next state or the output list of its fail state is NULL.

3.	void store_matches (pm_state_t *state, int i, dbllist_t * match_pattern_list):

•	(pm_state_t *state) - a pointer to a pm_state_t data structure
•	(int i) - an integer
•	(dbllist_t * match_pattern_list) - a pointer to a dbllist_t data structure

    the job of this function is to look for a match in the string with the pattern, and add the information about the match
    to the list of matches (match_pattern_list). It does this by iterating through the output list of the given state (state)
    and adding a pm_match_t data structure to the list of matches for each element in the output list.
    The pm_match_t data structure includes the pattern, the start and end positions of the match in the string,
    and a pointer to the state in the fsm where the match was found.


4.	void delete(pm_state_t *state):

•	(pm_state_t *state) - a pointer to a pm_state_t data structure

    the job of this function is to deallocate a pm_state_t data structure and all of its contents.
    It does this by deallocating the output and transitions lists of the
    state, recursively deallocating the fail state and all of its contents, and finally deallocating the state itself.


Compiling Steps:
•	gcc -Wall slist.c pattern_matching.c main.c -o main

Executing Stepps:
    If we don’t have arguments:
        •	./main
    With arguments:
        •	./main < “ argv[1] ” >
