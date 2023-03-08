Authored information’s:
•	Name: Hakam Nabulssi 


File information: 
•	Exercise Name: HTTP Client 
•	Submit Files: README.txt, client.c

Files content: 
•	client.c: this file contain the code that simulate a client server that constructs an HTTP request based on the user’s command line input, sends the request to a Web server,
    receives the reply from the server, and displays the reply    message on the screen  

•	ReadMe.txt: a description of the code in the files mentioned above and its functions “main and private”. 

Main functions of client.c file: 
•   Main: the main function constructs an http request according to the command, the command might contain arguments or text and URL, parse the command and the URL, 
    checks if the URL has a port number and a path, the default port number is 80, and construct the request (POST, GET) by the argument and the text took by the 
    command from the argv[] from  the command line.

Helper Functions in client.c:
•   int socket_connect(char * host, in_port_t port): function used to build and connect a socket, and define the host name, address family that is used to 
    designate the type of addresses that your socket can communicate with, and connect the socket.

•   void wrong_command_usage(): function to print a wrong usage message when its needed
    int parse_url(char * url, int port, char ip[], char path[]):function made to parse given URL path and all, checks if the url 
    starts with (http:// ), check the validation of the port number , and split the path.

Compiling steps: 
•   gcc -Wall -o client client.c

Executing steps:
•   example:  ./client -r 3 pr1=val1 pr2=val2 pr3=val3 http://www.testingmcafeesites.com/testcat_be.html -p 6 atexta


