#include "slist.h"

#include <string.h>

#include <stdlib.h>

#include "pattern_matching.h"

void dbllist_init(dbllist_t *list) {
    // Check if the input pointer is NULL and return immediately if it is
    if (list == NULL) {
        return;
    }
    // Set the head and tail pointers of the list to NULL to indicate that the list is empty
    dbllist_head(list) = NULL;
    dbllist_tail(list) = NULL;
    // Set the size of the list to 0 to reflect that the list is empty
    dbllist_size(list) = 0;
}

void dbllist_destroy(dbllist_t *list, dbllist_destroy_t dealloc) {
    // Check if the input dbllist_t pointer is NULL and return immediately if it is
    if (list == NULL) {
        return;
    }
    // Iterate over the list, starting from the head, using a while loop
    dbllist_node_t *current = dbllist_head(list);
    while (current != NULL) {
        // Free the memory allocated for the current node
        dbllist_node_t *next = dbllist_next(current);
        if (dealloc == DBLLIST_FREE_DATA) {
            // If the dealloc value is DBLLIST_FREE_DATA, also free the memory allocated for the data stored in the node
            void *data = dbllist_data(current);
            if (data != NULL) {
                free(data);
            }
        }
        free(current);
        // Set the current node pointer to the next node in the list and continue the while loop until all nodes have been processed
        current = next;
    }
}

int dbllist_append(dbllist_t *list, void *data) {
    // Check if the input dbllist_t pointer is NULL and return -1 if it is
    if (list == NULL) {
        return -1;
    }
    // Allocate memory for the new node using malloc
    dbllist_node_t *node = malloc(sizeof(dbllist_node_t));
    // If the allocation fails, return -1
    if (node == NULL) {
        return -1;
    }
    // Set the data and next pointers of the new node to the specified data and NULL, respectively
    dbllist_data(node) = data;
    dbllist_next(node) = NULL;
    // Set the previous pointer of the new node to the current tail of the list
    dbllist_prev(node) = dbllist_tail(list);

    // If the list is empty, set the head of the list to the new node
    if (dbllist_size(list) == 0) {
        dbllist_head(list) = node;
    }
        // Otherwise, set the next pointer of the current tail of the list to the new node
    else {
        dbllist_next(dbllist_tail(list)) = node;
    }
    // Update the tail of the list to the new node and increment the size of the list by 1
    dbllist_tail(list) = node;
    dbllist_size(list)++;

    return 0;
}

int dbllist_prepend(dbllist_t *list, void *data) {
    // Allocate memory for the new node
    dbllist_node_t *new_node = malloc(sizeof(dbllist_node_t));
    // If the allocation fails, return -1
    if (new_node == NULL) {
        return -1;
    }
    // Initialize the new node
    dbllist_data(new_node) = data;
    dbllist_prev(new_node) = NULL;
    dbllist_next(new_node) = dbllist_head(list);
    // Update the head of the list
    if (dbllist_head(list) != NULL) {
        dbllist_prev(dbllist_head(list)) = new_node;
    }
    dbllist_head(list) = new_node;
    // Update the tail of the list if the list was empty
    if (dbllist_tail(list) == NULL) {
        dbllist_tail(list) = new_node;
    }
    // Increase the size of the list
    dbllist_size(list)++;
    return 0;
}

int dbllist_remove(dbllist_t *list, dbllist_node_t *node, dbllist_destroy_t dealloc) {
    // Check if the node is in the list
    if (dbllist_prev(node) == NULL && dbllist_next(node) == NULL && dbllist_head(list) != node &&
        dbllist_tail(list) != node) {
        // The node is not in the list, return -1
        return -1;
    }
    // Update the pointers of the surrounding nodes
    if (dbllist_prev(node) != NULL) {
        dbllist_next(dbllist_prev(node)) = dbllist_next(node);
    }
    if (dbllist_next(node) != NULL) {
        dbllist_prev(dbllist_next(node)) = dbllist_prev(node);
    }
    // Update the head and tail of the list if necessary
    if (dbllist_head(list) == node) {
        dbllist_head(list) = dbllist_next(node);
    }
    if (dbllist_tail(list) == node) {
        dbllist_tail(list) = dbllist_prev(node);
    }
    // Decrease the size of the list
    dbllist_size(list)--;
    // Deallocate the node and its data if requested
    if (dealloc == DBLLIST_FREE_DATA) {
        free(dbllist_data(node));
    }
    free(node);
    return 0;
}