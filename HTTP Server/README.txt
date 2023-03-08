Authored information’s:
•	Name: Hakam Nabulssi 

File information: 
•	Exercise Name: HTTP server
•	Submit Files: README.txt, server.c, threadpool.c

Files content: 
•	server.c: In summary, you will be creating two source files, server.c and threadpool.c. The server file will handle connections with clients using TCP, by creating a socket for each client it talks to. To enable a multithreaded program, the server will create threads to handle these connections, and to maintain a limited number of threads, it will construct a thread pool
•	threadpool.c: thread pool will consist of a pre-created set of threads, and each time a client connection request is made, the server will enqueue the request for an available thread in the pool to handle it.
•	ReadMe.txt: a description of the code in the files mentioned above and its functions “main and private”. 

Main functions in threadpool.c: 
create_threadpool: This function creates a thread pool of a specified number of threads and initializes the necessary data structures and mutexes. It also starts each thread running the do_work function.
do_work: This function runs in an infinite loop and is executed by each thread in the thread pool. It waits for tasks to be added to the queue, and when a task is available, it retrieves it from the queue and executes it. The function also checks for shutdown conditions and exits the thread when appropriate.
dispatch: This function adds a new task to the thread pool's task queue for execution by the threads.
destroy_threadpool: This function is used to destroy the threadpool and clean up all the resources allocated to the threadpool.



Main functions in server.c:

•	Main : This function is the entry point for a server application that creates a thread pool and listens for incoming client connections on a specified port. The function takes command line arguments for the port number, thread pool size, and maximum number of concurrent client connections, and uses these arguments to set up the server socket, create a thread pool, and dispatch threads to handle incoming client requests.

•	get_mime_type: Determines the MIME type of a file based on its extension.

•	wrong_command_usage: Prints a usage message for an invalid command and exits the program.

•	set_http_response: Sets the HTTP status reason phrase and response body based on the HTTP status code passed as
an argument. This function takes three arguments: the HTTP status code, a pointer to a string representing the HTTP status reason phrase, and a pointer to a string representing the HTTP response body. The function uses a switch statement to set the reason phrase and response body based on the HTTP status code. This function is commonly used in web servers to generate HTTP responses with the appropriate status code, reason phrase, and response body. 

•	 parse_and_set_http_response: This function parses a request path and sets the appropriate HTTP response status code, reason phrase, and response body. It takes in a request path, pointers to an HTTP status code, status reason, and response body, as well as a buffer for the file path. The function first checks the request path for validity and extracts the file path from it. It then sets the HTTP status code, reason phrase, and response body based on the file path and HTTP version. If there is an error in parsing the request path, the function sets the HTTP status code to 400 Bad Request.

•	check_file_permissions: This function checks the permissions of a requested file to determine if it can be accessed by the server. It extracts the file extension from the root path and performs a series of checks on the file, including whether it exists, is readable by the server, and is executable by the user, group, and others. It sets the appropriate HTTP status code and response message based on the outcome of these checks.

•	 generate_directory_listing: This function generates a directory listing for a given directory by iterating through each file in the directory and constructing an HTML-formatted string that contains file names, last modified times, and file sizes (if it's not a directory). The output is returned as a dynamically allocated string.

•	 render_http_response_html: A function that fills in an HTML template with the provided HTTP status code, status reason, and response body. The filled-in HTML string is stored in the provided 'html' buffer.

•	 generate_http_response: This function generates an HTTP response as a string based on the provided status code, status reason, file path, file size, and other parameters. It includes the HTTP headers and body, as well as the current date and time, and returns the entire response as a string.

•	 process_client_request: function takes in a void pointer representing the client socket file descriptor and reads the client's HTTP request from that socket. It then parses the request to determine the requested resource's location and type (file or directory) and prepares an appropriate HTTP response for that resource. If the requested resource is a file, it reads the file's contents and adds them to the HTTP response. If the requested resource is a directory, it generates a directory listing and adds it to the HTTP response. Finally, it sends the HTTP response back to the client via the client socket.


Compiling steps:  gcc -o server server.c threadpool.c -lpthread

Executing steps: ./ server <port> <pool-size> <max-number-of-request>	

Testing:  http://<computer-name>:<port-num>/<your-path>



