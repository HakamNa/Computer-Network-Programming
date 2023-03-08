Authored information’s:
•	Name: Hakam Nabulssi 


File information: 
•	Exercise Name: Event-Driven Chat Server
•	Submit Files: README.txt, chatServer.c


Files content: 
•	chatServer.c: this file contain an implementation of event-driven chat server that forwards incoming messages to all connected clients except for the client that sent the message. The server should use the "select" function to check which sockets are ready for reading or writing, and should call "accept" when the main socket is ready for reading and "read" or "recv" when other sockets are ready for reading.

•	README.txt: a description of the code in the files mentioned above and its functions “main and private”. 


Functions of chatServer.c file: 
•	main: this function is the entry point of the program. It initializes the server, checks input arguments, sets the signal handler, creates and configures the socket, waits for incoming connections, and manages the data flow between clients. It reads and writes data from the sockets, and handles any errors that might occur. It also uses the select function to determine which sockets are ready for reading or writing and uses the FD_SET macro to add/remove file descriptors to the sets.

•	init_pool, initializes the connection pool by clearing all file descriptor sets, setting maxfd to -1, setting nready to 0, setting conn_head to NULL, and setting nr_conns to 0. It also resets all the file descriptor sets and sets the max fd to -1 indicating no active connections yet.

•	add_conn, function that adds a new connection to the connection pool. It allocates memory for a new connection, updates the max file descriptor if necessary, initializes values for the new connection such as the file descriptor, and the head and tail of the write message queue. It adds the new connection to the head of the list of connections in the pool and increments the number of connections.

•	remove_conn, function that removes a connection from the connection pool. It finds the connection to remove by searching the linked list of connections for a connection with a matching file descriptor. It updates the pointers in the linked list, clears the socket descriptor from the sets, frees the memory allocated for messages, decrement the number of connections, closes the socket descriptor, and finally frees the memory allocated for the connection.

•	add_msg, function that adds a new message to a connection's message queue in the connection pool. It finds the connection with the specified socket descriptor, creates a new message, allocates memory for it, copies the buffer message to the new message, adds the new message to the tail of the message queue, and returns success if the connection is found, otherwise it returns failure.

•	write_to_client, function that writes messages from a connection's message queue to the client. It finds the connection with the specified socket descriptor and writes messages in the queue to the client. It handles the case where not all data is written, updates the message to write remaining data. It removes the message from the queue if it was written successfully. It clears the socket descriptor from the sets if the message queue is empty. It returns success if the connection is found, otherwise it returns failure.

•	sig_handler, signal handling function that handles incoming signals. It sets a global variable to indicate that the server should end. If the signal received is a SIGINT, it returns without taking any further action.


Compiling steps: gcc -o chatServer chatServer.c

Executing steps (without valgrind ): ./chatServer <port_number>

Executing steps (with valgrind ): valgrind --leak-check=full --show-leak-kinds=all ./chatServer <port_number>

For connections: telnet localhost <port_number>  or nc localhost <port_number> 



