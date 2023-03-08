#include "chatServer.h"

#include <sys/socket.h>

#include <sys/signal.h>

#include <stdio.h>

#include <stdlib.h>

#include <netinet/in.h>

#include <sys/ioctl.h>

#include <unistd.h>

#include <string.h>

#include <ctype.h>

#include <stdint.h>

#include <errno.h>

#define FALSE 0
static int end_server = FALSE;

// Signal handler function
void sig_handler(int signum) {
    // Set global variable to indicate that the server should end
    end_server = !FALSE;
    // If the signal received is a SIGINT, return without taking any further action
    if (signum == SIGINT) {
        return;
    }
}

// main function that starts the server and listens for incoming connections
int main(int argc, char *argv[]) {
    int fd, client_fd, read_flag, on = 1, port_len;
    struct sockaddr_in srv_add, cli_add;
    char buffer[BUFFER_SIZE + 1] = {
            0
    };
    char *port_str, *end_ptr;
    long int port;
    socklen_t addr_len;

    // check if the correct number of arguments are passed
    if (argc != 2) {
        printf("Usage: server <port>\n");
        exit(EXIT_FAILURE);
    }

    // check if the passed argument is a valid port number
    port_str = argv[1];
    port_len = (int) strlen(port_str);
    for (int i = 0; i < port_len; i++) {
        if (!isdigit(port_str[i])) {
            printf("Usage: server <port>\n");
            exit(EXIT_FAILURE);
        }
    }

    port = strtol(argv[1], &end_ptr, 10);
    if (*end_ptr != '\0' || port < 1 || port > UINT16_MAX) {
        printf("Usage: server <port>\n");
        exit(EXIT_FAILURE);
    }

    // set the signal handler for interruption
    signal(SIGINT, sig_handler);

    // allocate memory for the connection pool
    conn_pool_t *pool = calloc(1, sizeof(conn_pool_t));
    if (pool == NULL) {
        perror("ERROR:allocating memory\n");
        exit(EXIT_FAILURE);
    }
    // set the server address and port
    srv_add.sin_addr.s_addr = htonl(INADDR_ANY);
    srv_add.sin_family = AF_INET;
    srv_add.sin_port = htons(atoi(argv[1]));

    // ************************* INITIALIZE CONNECTION POOL *************************
    init_pool(pool);

    // ************************* CREATE SOCKET *************************
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("ERROR:socket\n");
        exit(EXIT_FAILURE);
    }

    // ************************* SET SOCKET TO NON_BLOCKING *************************
    if (ioctl(fd, (int) FIONBIO, (char *) &on) < 0) {
        perror("ERROR:ioctl\n");
        close(fd);
        exit(EXIT_FAILURE);
    }
    // ************************* BIND THE SOCKET *************************
    if ((bind(fd, (struct sockaddr *) &srv_add, sizeof(srv_add))) < 0) {
        perror("ERROR:bind\n");
        close(fd);
        exit(EXIT_FAILURE);
    }
    // ************************* LISTEN ON THE SOCKET *************************
    if ((listen(fd, 5)) < 0) {
        perror("ERROR:listen\n");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // initialize file descriptor sets
    FD_SET(fd, &(pool->read_set));
    pool->maxfd = fd;
    addr_len = sizeof(cli_add);

    // loop to wait for incoming connections, data, or to write data
    do {
        // copy the master fd_set to the working fd_set
        FD_ZERO(&pool->ready_read_set);
        FD_ZERO(&pool->ready_write_set);
        pool->ready_read_set = pool->read_set;
        pool->ready_write_set = pool->write_set;

        // ************************* WAIT FOR ACTIVITY FOR SOCKETS *************************
        printf("Waiting on select()...\nMaxFd %d\n", pool->maxfd);
        if ((pool->nready = select(pool->maxfd + 1, &(pool->ready_read_set), &(pool->ready_write_set), NULL, NULL)) <
            0) {
            if (end_server == FALSE)
                perror("ERROR:select\n");
            break;
        }
        // iterate through the set of sockets with activity
        for (int descriptor_index = 0, processed_count = 0; processed_count < pool->nready; descriptor_index++) {
            if (pool->nready == 0) {
                break;
            }
            // check to see if this descriptor is ready for read
            if (FD_ISSET(descriptor_index, &(pool->ready_read_set))) {
                // if this is the listening socket, accept one incoming connection
                FD_CLR(descriptor_index, &pool->ready_read_set);
                if (descriptor_index == fd) {
                    // ************************* ACCEPT INCOMING CONNECTION *************************
                    client_fd = accept(fd, (struct sockaddr *) &cli_add, &addr_len);
                    if (client_fd > pool->maxfd) {
                        pool->maxfd = client_fd;
                    } else if (client_fd < 0) {
                        continue;
                    }
                    printf("New incoming connection on sd %d\n", client_fd);
                    FD_SET(client_fd, &(pool->read_set));
                    // ************************* ADD NEW CONNECTION TO THE POOL *************************
                    if (add_conn(client_fd, pool) == -1) {
                        close(client_fd);
                    }
                    continue;
                } else {
                    // if this is not the listening socket, an existing connection must be readable
                    printf("Descriptor %d is readable\n", descriptor_index);
                    memset(buffer, '\0', BUFFER_SIZE + 1);

                    // ************************* READ DATA FROM THE CLIENT *************************
                    if ((read_flag = (int) read(descriptor_index, buffer, BUFFER_SIZE)) < 0) {
                        continue;
                    }
                    printf("%d bytes received from sd %d\n", read_flag, descriptor_index);
                    // if the connection has been closed by client, remove the connection
                    if (read_flag == 0) {
                        printf("Connection closed for sd %d\n", descriptor_index);
                        // ************************* REMOVE CLOSED CONNECTION FROM THE POOL *************************
                        if (remove_conn(descriptor_index, pool) == -1) {
                            continue;
                        }
                        if (pool->maxfd == descriptor_index) {
                            pool->maxfd = (int) pool->nr_conns + 3;
                        }
                        close(descriptor_index);
                        continue;
                    } else if (read_flag < 0) {
                        if (errno == EINTR) {
                            continue;
                        } else {
                            perror("ERROR:read\n");
                            exit(EXIT_FAILURE);
                        }
                    }
                    // otherwise, process the data received from the client
                    for (conn_t *ad = pool->conn_head; ad != NULL; ad = ad->next) {
                        // ************************* ADD RECEIVED DATA TO MESSAGE QUEUE *************************
                        if (ad->fd != descriptor_index) {
                            add_msg(ad->fd, buffer, read_flag, pool);
                            FD_SET(ad->fd, &(pool->write_set));
                        }
                    }
                }
                processed_count++;
            } // End of if (FD_ISSET())
            // ************************* WRITE DATA TO CLIENT *************************
            if (FD_ISSET(descriptor_index, &(pool->ready_write_set))) {
                // try to write all msgs in queue to sd
                write_to_client(descriptor_index, pool);
                processed_count++;
                FD_CLR(descriptor_index, &(pool->write_set));

            }

        }

    } while (end_server == FALSE);
    // ************************* CLEAN UP OPEN CONNECTIONS AND FREE MEMORY ON EXIT *************************

    for (conn_t *current = pool->conn_head; current != NULL;) {
        for (msg_t *current_message = current->write_msg_head; current_message != NULL;) {
            msg_t *temp_msg = current_message;
            current_message = current_message->next;
            free(temp_msg->message);
            free(temp_msg);
        }
        close(current->fd);
        conn_t *temp_conn = current;
        current = current->next;
        free(temp_conn);
    }
    // remove file descriptor from all fd sets
    FD_CLR(fd, &pool->ready_read_set);
    FD_CLR(fd, &pool->read_set);
    FD_CLR(fd, &pool->ready_write_set);
    FD_CLR(fd, &pool->write_set);
    close(fd);
    free(pool);

    return EXIT_SUCCESS;
}

/*
 * Init the conn_pool_t structure.
 * @pool - allocated pool
 * @ return value - 0 on success, -1 on failure
 */

int init_pool(conn_pool_t *pool) {
    //initializes a connection pool by clearing all file descriptor sets
    FD_ZERO(&pool->read_set);
    FD_ZERO(&pool->ready_read_set);
    FD_ZERO(&pool->write_set);
    FD_ZERO(&pool->ready_write_set);

    // set maxfd to -1 to indicate that there are no active connections yet
    pool->maxfd = -1;
    pool->nready = 0;
    pool->conn_head = NULL;
    pool->nr_conns = 0;

    return EXIT_SUCCESS;
}

/*
 * Add connection when new client connects the server.
 * @ sd - the socket descriptor returned from accept
 * @pool - the pool
 * @ return value - 0 on success, -1 on failure
 */

int add_conn(int sd, conn_pool_t *pool) {
    conn_t *connect = calloc(1, sizeof(conn_t));
    if (connect == NULL) {
        perror("ERROR:allocating memory\n");
        return EXIT_FAILURE;
    }
    // update maxfd if necessary
    if (sd > pool->maxfd) {
        pool->maxfd = sd;
    }
    //initialize values for new connection
    connect->next = NULL;
    connect->prev = NULL;
    connect->fd = sd;
    connect->write_msg_head = NULL;
    connect->write_msg_tail = NULL;
    //add new connection to the head of the list
    if (pool->conn_head == NULL) {
        pool->conn_head = connect;
    } else {
        connect->next = pool->conn_head;
        pool->conn_head->prev = connect;
        pool->conn_head = connect;
    }
    // increment number of connections
    pool->nr_conns += 1;
    return EXIT_SUCCESS;
}

/*
 * Remove connection when a client closes connection, or clean memory if server stops.
 * @ sd - the socket descriptor of the connection to remove
 * @pool - the pool
 * @ return value - 0 on success, -1 on failure
 */

int remove_conn(int sd, conn_pool_t *pool) {

    conn_t *current_connection = pool->conn_head;
    conn_t *connection_to_remove = NULL;

    // find the connection to remove
    while (current_connection != NULL) {
        if (current_connection->fd == sd) {
            connection_to_remove = current_connection;
            break;
        }
        current_connection = current_connection->next;
    }

    // connection not found
    if (connection_to_remove == NULL) {
        return EXIT_FAILURE;
    }

    // update pointers in the linked list
    if (connection_to_remove == pool->conn_head) {
        pool->conn_head = connection_to_remove->next;
    }
    if (connection_to_remove->prev != NULL) {
        connection_to_remove->prev->next = connection_to_remove->next;
    }
    if (connection_to_remove->next != NULL) {
        connection_to_remove->next->prev = connection_to_remove->prev;
    }

    // clear the socket descriptor from the sets
    FD_CLR(sd, &pool->read_set);
    FD_CLR(sd, &pool->write_set);
    FD_CLR(sd, &pool->ready_read_set);
    FD_CLR(sd, &pool->ready_write_set);

    // free the memory allocated for messages
    msg_t *current_message = connection_to_remove->write_msg_head;
    while (current_message != NULL) {
        msg_t *message_to_free = current_message;
        current_message = current_message->next;
        free(message_to_free->message);
        free(message_to_free);
    }
    printf("Removing connection with socket descriptor %d\n", sd);

    // free the memory allocated for the connection
    free(connection_to_remove);
    pool->nr_conns--;

    // close the socket descriptor
    FD_CLR(sd, &pool->read_set);
    FD_CLR(sd, &pool->write_set);
    FD_CLR(sd, &pool->ready_write_set);
    FD_CLR(sd, &pool->ready_read_set);
    return EXIT_SUCCESS;
}

/*
 * Add msg to the queues of all connections (except of the origin).
 * @ sd - the socket descriptor to add this msg to the queue in its conn object
 * @ buffer - the msg to add
 * @ len - length of msg
 * @pool - the pool
 * @ return value - 0 on success, -1 on failure
 */

int add_msg(int sd, char *buffer, int len, conn_pool_t *pool) {
    conn_t *find = NULL;
    //find the connection with the specified socket descriptor
    for (conn_t *connection = pool->conn_head; connection != NULL; connection = connection->next) {
        if (connection->fd == sd) {
            find = connection;
            break;
        }
    }
    //if the connection is found
    if (find) {
        msg_t *new_message = (msg_t *) calloc(1, sizeof(msg_t));
        if (new_message == NULL) {
            perror("ERROR:allocating memory\n");
            return EXIT_FAILURE;
        }
        //allocate memory for the new message
        new_message->message = (char *) calloc(1, sizeof(char) * (1 + len));
        if (new_message->message == NULL) {
            perror("ERROR:allocating memory\n");
            return EXIT_FAILURE;
        }
        //copy the buffer message to the new message
        strcpy(new_message->message, buffer);
        new_message->size = len;
        new_message->next = NULL;
        new_message->prev = NULL;
        // add the new message to the tail of the message queue
        if (find->write_msg_head == NULL) {
            find->write_msg_head = new_message;
            find->write_msg_tail = new_message;
        } else {
            find->write_msg_tail->next = new_message;
            new_message->prev = find->write_msg_tail;
        }
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}

/*
 * Write msg to client.
 * @ sd - the socket descriptor of the connection to write msg to
 * @pool - the pool
 * @ return value - 0 on success, -1 on failure
 */

int write_to_client(int sd, conn_pool_t *pool) {
    int amount_sent;
    //find the connection with the specified socket descriptor
    for (conn_t *connection = pool->conn_head; connection != NULL; connection = connection->next) {
        if (connection->fd == sd) {
            msg_t *current_message = connection->write_msg_head;
            //write messages in the queue to the client
            while (current_message != NULL) {
                amount_sent = (int) write(sd, current_message->message, current_message->size);
                if (amount_sent < 0) {
                    perror("ERROR:write\n");
                    return EXIT_FAILURE;
                }
                //if not all data was written, update the message to write remaining data
                if (amount_sent < current_message->size) {
                    current_message->message += amount_sent;
                    current_message->size -= amount_sent;
                    break;
                }
                // message written successfully, remove it from the queue
                msg_t *removed_message = current_message;
                current_message = current_message->next;
                free(removed_message->message);
                free(removed_message);
                connection->write_msg_head = current_message;
            }
            if (connection->write_msg_head == NULL) {
                connection->write_msg_tail = NULL;
            }
            //clear the socket descriptor from the sets
            FD_CLR(sd, &pool->write_set);
            FD_CLR(sd, &pool->ready_write_set);
            return EXIT_SUCCESS;
        }
    }
    return EXIT_FAILURE;
}

