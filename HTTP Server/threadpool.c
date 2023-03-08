#include <stdio.h>

#include <stdlib.h>

#include <unistd.h>

#include "threadpool.h"

threadpool *create_threadpool(int num_threads_in_pool) {
    // Check if the number of threads requested is valid
    if (num_threads_in_pool > MAXT_IN_POOL || num_threads_in_pool <= 0) {
        return NULL;
    }
    // Allocate memory for the thread pool structure
    threadpool *pool = (threadpool *) malloc(sizeof(threadpool));

    // Check if memory allocation was successful
    if (pool == NULL) {
        fprintf(stdin, "Error: Failed to allocate memory \n");
        return NULL;
    } else {
        // Allocate memory for the thread IDs
        pool->threads = (pthread_t *) malloc(num_threads_in_pool * sizeof(pthread_t));
        // Check if memory allocation was successful
        if (pool->threads == NULL) {
            fprintf(stdin, "Error: Failed to allocate memory \n");
            free(pool);
            return NULL;
        } else {
            // Initialize thread pool fields
            pool->qsize = 0;
            pool->dont_accept = 0;
            pool->shutdown = 0;
            pool->num_threads = num_threads_in_pool;
            pool->qhead = NULL;
            pool->qtail = NULL;

            // Initialize mutex and condition variables
            if (pthread_mutex_init(&(pool->qlock), NULL) != 0) {
                fprintf(stderr, "Error: Failed to initialize mutex.\n");
                free(pool->threads);
                free(pool);
                return NULL;
            }
            if (pthread_cond_init(&(pool->q_empty), NULL) != 0) {
                fprintf(stderr, "Error: Failed to initialize condition 'q_empty'.\n");
                free(pool->threads);
                pthread_mutex_destroy(&pool->qlock);
                free(pool);
                return NULL;
            }
            if (pthread_cond_init(&(pool->q_not_empty), NULL) != 0) {
                fprintf(stderr, "Error: Failed to initialize condition 'q_not_empty'.\n");
                free(pool->threads);
                pthread_mutex_destroy(&pool->qlock);
                pthread_cond_destroy(&pool->q_empty);
                free(pool);
                return NULL;
            }

            // Create worker threads
            for (pthread_t *thread = pool->threads; thread < pool->threads + num_threads_in_pool; ++thread) {
                if (pthread_create(thread, NULL, do_work, pool) != 0) {
                    // Thread creation failed, cleanup and return NULL
                    fprintf(stdin, "Error: unable to create thread %ld\n", thread - pool->threads);
                    for (pthread_t *t = pool->threads; t < thread; ++t) {
                        pthread_cancel(*t);
                    }
                    free(pool->threads);
                    pthread_mutex_destroy(&pool->qlock);
                    pthread_cond_destroy(&pool->q_empty);
                    pthread_cond_destroy(&pool->q_not_empty);
                    free(pool);
                    return NULL;
                }
            }
        }
    }
    return pool;
}

void dispatch(threadpool *from_me, dispatch_fn dispatch_to_here, void *arg) {
    // If the thread pool is not accepting new work, do nothing
    if (from_me->dont_accept == 1) {
        return;
    }

    // Allocate memory for a new work item
    work_t *element = malloc(sizeof(work_t));
    if (element == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for element item.\n");
        // If memory allocation failed, clean up the thread pool and return
        goto cleanup_pool;
    }

    // Set the argument and function to execute for the new work item
    element->arg = arg;
    element->routine = dispatch_to_here;

    // Acquire the lock on the thread pool's queue
    pthread_mutex_lock(&from_me->qlock);

    // Add the new work item to the end of the queue
    if (from_me->qhead == NULL) {
        from_me->qhead = element;
    } else {
        from_me->qtail->next = element;
    }
    from_me->qtail = element;
    from_me->qsize++;

    // Signal that the queue is no longer empty
    if (pthread_cond_signal(&from_me->q_not_empty) != 0) {
        fprintf(stderr, "Error: Failed to signal condition variable.\n");
        // If signaling the condition variable failed, clean up the work item and thread pool and return
        goto cleanup_work;
    }

    // Release the lock on the thread pool's queue and return
    pthread_mutex_unlock(&from_me->qlock);
    return;

    // Clean up the work item and thread pool
    cleanup_work:
    free(element);
    cleanup_pool:
    free(from_me->threads);
    pthread_mutex_destroy(&from_me->qlock);
    pthread_cond_destroy(&from_me->q_empty);
    pthread_cond_destroy(&from_me->q_not_empty);
    free(from_me);
}

void *do_work(void *p) {
    // Cast the void pointer argument to a threadpool pointer
    threadpool *pool = (threadpool *) p;

    work_t *element;

    // Enter an infinite loop to continuously execute work items
    while (1) {
        // Acquire the lock on the thread pool's queue
        pthread_mutex_lock(&pool->qlock);

        // If the queue is empty, wait for a signal that new work has been added
        while (pool->qsize == 0) {
            // If the thread pool has been shut down, release the lock and exit the function
            if (pool->shutdown) {
                pthread_mutex_unlock(&pool->qlock);
                return NULL;
            }
            // Wait for a signal that the queue is no longer empty
            pthread_cond_wait(&pool->q_not_empty, &pool->qlock);
        }

        // Get the first work item from the queue
        element = pool->qhead;
        pool->qsize--;

        // If the queue is now empty, reset the queue pointers
        if (pool->qsize == 0) {
            pool->qhead = NULL;
            pool->qtail = NULL;
        } else {
            pool->qhead = element->next;
        }

        // If the queue is now empty and the thread pool is not accepting new work, signal that the queue is empty
        if (pool->qsize == 0 && pool->dont_accept) {
            pthread_cond_signal(&pool->q_empty);
        }

        // Release the lock on the queue
        pthread_mutex_unlock(&pool->qlock);

        // Execute the work item's routine function with its argument
        (element->routine)(element->arg);

        // Free the memory allocated for the work item
        free(element);
    }
}

void destroy_threadpool(threadpool *destroyme) {
    // Mark the pool as not accepting new work
    destroyme->dont_accept = 1;

    // Lock the queue and wait for it to become empty
    pthread_mutex_lock(&destroyme->qlock);
    while (destroyme->qsize != 0) {
        pthread_cond_wait(&destroyme->q_empty, &destroyme->qlock);
    }

    // Mark the pool as shutting down and signal all threads
    destroyme->shutdown = 1;
    pthread_cond_broadcast(&destroyme->q_not_empty);

    // Unlock the queue and join all threads
    pthread_mutex_unlock(&destroyme->qlock);
    for (int i = 0; i < destroyme->num_threads; i++) {
        if (pthread_join(destroyme->threads[i], NULL) != 0) {
            fprintf(stderr, "Error: failed to join thread %d\n", i);
        }
    }

    // Destroy the synchronization primitives and free memory
    pthread_cond_destroy(&destroyme->q_empty);
    pthread_cond_destroy(&destroyme->q_not_empty);
    pthread_mutex_destroy(&destroyme->qlock);
    free(destroyme->threads);
    free(destroyme);
}