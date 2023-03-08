// Name : Hakam Nabulssi

// includes and libraries
#include <stdio.h>

#include <string.h>

#include <stdlib.h>

#include <math.h>

#include <unistd.h>

#include <stdbool.h>

#include <sys/socket.h>

#include <netinet/in.h>

#include <netdb.h>

#define BUFFER_SIZE 65536

int socket_connect(char *host, in_port_t port); // function used to build and connect a socket
void wrong_command_usage(char *[], int); // function to print a wrong usage message when its needed
int parse_url(char *url, int port, char ip[], char path[], char *[],
              int argc); // function made to parse given url and takes the path and all the args from it

//  building a socket
int socket_connect(char *host, in_port_t port) {
    struct hostent *hp = gethostbyname(host);
    struct sockaddr_in addr;
    int sock;
    // checks if the host name is legal
    if (hp == NULL) {
        herror("gethostbyname");
        exit(1);
    }
    // getting the ip
    bcopy(hp->h_addr, &addr.sin_addr, hp->h_length);
    // getting the port
    addr.sin_port = htons(port);
    // the family type
    addr.sin_family = AF_INET;
    // building  the socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    // checks the socket's status
    if (sock == -1) {
        perror("socket");
        exit(1);
    }
    // connect the socket
    if (connect(sock, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) == -1) {
        perror("connect");
        exit(1);
    }
    return sock;
}

void wrong_command_usage(char *usage[], int argc) {
    char usage_message[BUFFER_SIZE] = "";
    int len = 0;
    for (int i = 1; i < argc; i++) {
        len += snprintf(usage_message + len, sizeof(usage_message) - len, "%s ", usage[i]);
    }
    printf("Usage: client %s \n", usage_message);
    exit(-1);
}

int parse_url(char *url, int port, char ip[], char path[], char *usage[], int argc) {
    int len;
    bool port_checker = false;
    // checks if starts with http
    if (!((url)[0] == 'h' &&
          (url)[1] == 't' &&
          (url)[2] == 't' &&
          (url)[3] == 'p' &&
          (url)[4] == ':' &&
          (url)[5] == '/' &&
          (url)[6] == '/')) {
        wrong_command_usage(usage, argc);
    }
    len = (int) strlen(url);
    char word[len];
    // copying the url to an array
    strcpy(word, url);
    // checks if the url has a port
    for (int i = 5; i < len; i++) {
        if (word[i] == ':') {
            port_checker = true;
        }
    }
    // parsing the url
    if (port_checker == true) {
        // if the url has a port number
        sscanf(url, "http://%99[^:]:%99d%99[^\n]", ip, &port, path);
    } else {
        //if the urls has not a port number
        sscanf(url, "http://%99[^/]%99[^\n]", ip, path);
    }
    // if the url has no path
    if (strcmp(path, "") == 0) {
        strcpy(path, "/");
    }
    // if port number isn't legal , wrong_usage
    if (port <= 0 || port >= pow(2, 16)) {
        wrong_command_usage(usage, argc);
    }
    return port;
}

int main(int argc, char *argv[]) {
    int size = 0, accept, fd, i, j, r_index = -1, p_index = -1, r_args_nums, p_text_length, port = 80, len =
            argc - 2, r_args_counter = 0;
    char buffer[BUFFER_SIZE], path[BUFFER_SIZE] = "", request[BUFFER_SIZE] = "", command[BUFFER_SIZE] = " ", args_text[BUFFER_SIZE] = " ", ip[BUFFER_SIZE];
    bool r_flag = false, p_flag = false;
    char *p_text = NULL, *url = NULL;
    char *args_container[BUFFER_SIZE] = {
            NULL
    }, *word[len];
    // if the command is invalid
    if (argc < 2) {
        wrong_command_usage(argv, argc);
    }
        // if the command contain only the url
    else if (argc == 2) {
        port = parse_url(argv[1], port, ip, path, argv, argc);
        // if the command has more than the url
    } else {
        // *****************************************************************  PARSING THE COMMAND *****************************************************************
        for (i = 0, j = 1; i <= len; i++, j++) {
            // copying the args of the command to word buffer
            word[i] = argv[j];
            // converting the command into string
            strcat(command, word[i]);
            strcat(command, " ");
            // check if the command has hyphen in valid way (with r,p or not)
            if (strcmp(word[i], "-") == 0) {
                wrong_command_usage(argv, argc);
                // if after the hyphen we have r, save the index
            } else if (strcmp(word[i], "-r") == 0) {
                r_index = i;
                // if after the hyphen we have p, save the index
            } else if (strcmp(word[i], "-p") == 0) {
                p_index = i;
            }
        }
        // if the command end with -r  or -p
        if (strcmp(word[len], "-r") == 0 || strcmp(word[len], "-p") == 0)
            wrong_command_usage(argv, argc);

        // checks the validity of r, p if found
        if (strcmp(word[r_index + 1], "0") == 0 || atoi(word[r_index + 1]) > 0)
            // if r is valid , turn on the flag
            r_flag = true;
        if (atoi(word[p_index + 1]) > 0)
            // if p is valid turn on the flag
            p_flag = true;
        // if r, p invalid (the flags are off )
        if (strcmp(word[p_index + 1], "0") == 0)
            wrong_command_usage(argv, argc);
        if ((r_index != -1 && r_flag == false))
            wrong_command_usage(argv, argc);
            // *****************************************************************  r CONDITIONS *****************************************************************
        else {
            // if r found
            for (i = 0; i <= len; i++) {
                // sign the index of r with r_index and convert the args num to int
                if (strstr(word[i], "-r") != NULL) {
                    r_args_nums = atoi(word[i + 1]);
                    r_index = i;
                }
            }
            // copying the args of r into container
            int p2 = r_index + 2;
            if (r_index != -1) {
                for (int p1 = 0; p1 < r_args_nums; ++p1) {
                    args_container[p1] = word[p2];
                    strcat(args_text, word[p2]);
                    strcat(args_text, " ");
                    p2++;
                }
            }
        }
        // validation of p
        if ((p_index != -1 && p_flag == false))
            wrong_command_usage(argv, argc);
            // *****************************************************************  p CONDITIONS *****************************************************************

        else {
            for (i = 0; i <= len; i++) {
                // converting the length of the text into int , splitting the text from the command
                if (strstr(word[i], "-p") != NULL) {
                    p_text_length = atoi(word[i + 1]);
                    p_text = word[i + 2];
                }
            }
            // if the given number of the length is different from the number before it , wrong_usage
            if (p_text != NULL && strlen(p_text) != p_text_length)
                wrong_command_usage(argv, argc);
        }
        // *****************************************************************  URL CONDITIONS *****************************************************************

        // checking the validity of the URL, and checks that starts with http://
        for (i = 0; i <= len; i++) {
            if (strstr(word[i], "http://") != NULL)
                url = word[i];
        }
        if (url == NULL)
            wrong_command_usage(argv, argc);
        // parse the URL and make sure that the number of the port is legal
        port = parse_url(url, port, ip, path, argv, argc);
        // *****************************************************************  P USAGE CONDITIONS *****************************************************************
        // making sure that all the variables in p if found are legal
        if ((p_text_length == 0 && p_text != NULL) || (p_text_length != 0 && p_text == NULL) ||
            (p_text != NULL && strcmp(p_text, url) == 0) || (p_text != NULL && strcmp(p_text, "-r") == 0))
            wrong_command_usage(argv, argc);
        // *****************************************************************  R USAGE CONDITIONS *****************************************************************

        // making sure that all the variables in r if found are legal
        for (int k = 0; k < r_args_nums; ++k) {
            char *ret = strchr(args_container[k], '=');
            if ((args_container[k] != NULL && ret == NULL) || args_container[k][0] == '=' ||
                (args_container[k][strlen(args_container[k]) - 1]) == '=')
                wrong_command_usage(argv, argc);
        }
        // counter for the args for checking if the number of args is equal to the given number on the command
        for (int k = r_index; k < len; ++k) {
            if (strchr(word[k], '=') != NULL && word[k] != url) {
                r_args_counter++;
            }
        }
        if (r_args_counter != r_args_nums)
            wrong_command_usage(argv, argc);
    }
    // *****************************************************************  BUILDING THE REQUEST *****************************************************************
    // the variables the header should contain

    char *header_GET = "GET ";
    char *header_POST = "POST ";
    char *header_path = path;
    char *header_host = ip;
    char *header_med = " HTTP/1.0\r\nHost: ";
    char *header_last = "\r\n\r\n";
    char *content_length_header = "\r\nContent-length:";
    // if there is no p neither r , build the request
    if (p_index == -1 && r_index == -1) {
        strcpy(request, header_GET); // GET
        strcat(request, header_path); // the path
        strcat(request, header_med); // HTTP 1.0
        strcat(request, header_host); // HOST
        strcat(request, header_last); // \r\n\r\n
        // if there is r but no p , build the request
    } else if (p_index == -1 && r_index != -1) {
        strcpy(request, header_GET); // GET
        strcat(request, header_path); // the path
        strcat(request, "?"); // ?
        int index = 0;
        while (args_container[index] != NULL) {
            strcat(request, args_container[index]); // the args
            if (index < r_args_nums - 1) // for removing the & after the last arg
                //                concat &
                strcat(request, "&");
            index++;
        }
        strcat(request, header_med); // HTTP 1.0
        strcat(request, header_host); // HOST
        strcat(request, header_last); // \r\n\r\n
        //  if there is p but no r
    } else if (p_index != -1 && r_index == -1) {
        strcpy(request, header_POST); // POST
        strcat(request, header_path); // the path
        strcat(request, header_med); //  HTTP 1.0
        strcat(request, header_host); // HOST
        strcat(request, content_length_header); // the length text
        strcat(request, word[p_index + 1]); // the length itself
        strcat(request, header_last); // \r\n\r\n
        strcat(request, p_text); // the text after -p
    } else {
        // if either -r and -p founded
        strcpy(request, header_POST); // POST
        strcat(request, header_path); // the path
        strcat(request, "?"); // ?
        int index = 0;
        // concat the args of r
        while (args_container[index] != NULL) {
            strcat(request, args_container[index]);
            // for removing the last &
            if (index < r_args_nums - 1)
                // concat &
                strcat(request, "&");
            index++;
        }
        strcat(request, header_med); // HTTP :1.0
        strcat(request, header_host); // HOST
        strcat(request, content_length_header); // length text
        strcat(request, word[p_index + 1]); // the length itself
        strcat(request, header_last); // \r\n\r\n
        strcat(request, p_text); // text of p
    }
    // print the construction of the http request before sending it
    printf("HTTP request =\n%s\nLEN = %d\n", request, ((int) strlen(request)));

    // *****************************************************************  STARTING THE CONNECTION *****************************************************************
    // start the connection with the socket
    fd = socket_connect(ip, port);
    // write to the file
    if (send(fd, request, strlen(request),0) == -1) {
        perror("write");
        exit(1);
    }

    // cleaning the buffer
    bzero(buffer, BUFFER_SIZE);
    // while read return 0 continue to read , else close the connection  and get out
    while (1) {
        // cleaning the buffer
        bzero(buffer, BUFFER_SIZE);
        // read from  the file
        accept = (int) recv(fd, buffer, BUFFER_SIZE, 0);
        // if read doesn't work then exit
        if (accept < 0) {
            perror("read");
            exit(1);
            // if its working , while it's not 0, continue to print on the screen
        } else if (accept != 0) {
            write(1, buffer, accept);
            size += accept;
        } else
            break;
    }
    // printing the length of the received response in the wanted form
    printf("\n Total received response bytes: %d\n", size);
    // closing the fd
    close(fd);

}
// ***************************************************************** END *****************************************************************

