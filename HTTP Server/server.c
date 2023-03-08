/*
 * NAME : Hakam Nabulssi
 */

#include "threadpool.h"

#include <stdio.h>

#include <stdlib.h>

#include <string.h>

#include <unistd.h>

#include <sys/socket.h>

#include <netinet/in.h>

#include <sys/stat.h>

#include <dirent.h>

#include <fcntl.h>

#include <stdbool.h>

// Date and Time Formats
#define RFC1123FMT "%a, %d %b %Y %H:%M:%S GMT"

// HTTP Content Types
#define TYPE "text/html"

// Default HTML File
#define ROOT_DIR "index.html"

// Error Messages
#define FOUND "Directories must end with a slash."
#define BAD_REQUEST "Bad Request."
#define FORBIDDEN "Access denied."
#define NOT_FOUND "File not found."
#define INTERNAL_SERVER_ERROR "Some server side error."
#define NOT_SUPPORTED "Method is not supported."

// HTTP Status Codes
#define OK "OK"

// HTML Templates
#define HTML_TEMPLATE "<HTML><HEAD><TITLE>%d %s</TITLE></HEAD>\r\n<BODY><H4>%d %s</H4>\r\n%s\r\n</BODY></HTML>\r\n"
#define DIRECTORY_HEADER "<HTML>\r\n<HEAD><TITLE>Index of %s</TITLE></HEAD>\r\n<BODY>\r\n<H4>Index of %s</H4>\r\n<table CELLSPACING=8>\r\n<tr><th>Name</th><th>Last Modified</th><th>Size</th></tr>\r\n"
#define CREATE_TABLE "<tr>\r\n<td><A HREF=\"%s\">%s</A></td><td>\r\n"
#define END_OF_PAGE "</table>\r\n<HR>\r\n<ADDRESS>webserver/1.0</ADDRESS>\r\n</BODY></HTML>\r\n\r\n"

// HTTP Response Headers
#define RESPONSE_HEADER "HTTP/1.1 %d %s\r\nServer: webserver/1.0\r\nDate: "
#define LOCATION "\r\nLocation: %s/\r\n"
#define CONTENT_TYPE "\r\nContent-Type: %s"
#define CONTENT_LENGTH "\r\nContent-Length: %d"
#define TIME "\r\nLast-Modified: %a, %d %b %Y %H:%M:%S GMT"
#define CLOSE_CONNECTION "\r\nConnection: close\r\n\r\n"
// Limits and Sizes
#define MAX_REQUEST_LENGTH 1024
#define RESPONSE_LENGTH (MAX_REQUEST_LENGTH / 2)
#define PATH_LENGTH (MAX_REQUEST_LENGTH * 2)
#define HTML_LENGTH (MAX_REQUEST_LENGTH / 4)

// This function takes a file name as input and returns the corresponding MIME type (a string indicating the format of the data in the file).
char *get_mime_type(char *);

// This function prints an error message to stdout and exits the program if a command is used incorrectly.
void wrong_command_usage(char *[], int);

// This function sets the HTTP response status code, reason phrase, and response body based on the provided status code.
void set_http_response(int, char **, char **);

// This function parses the given request path and sets the HTTP response code, reason, and body.
void parse_and_set_http_response(char *, int *, char **, char **, char *);

// Check file permissions for a given file extension and root path.
// It also checks whether the file is readable by the user, group, and others,or whether a directory is executable by others.
void check_file_permissions(char *, char *, struct stat *, int *, char **, char **);

// This function generates a directory listing of the files in a directory specified by 'index_file_path' parameter
char *generate_directory_listing(char *, size_t, char *, struct dirent *, struct stat *, DIR *);

//generate an HTML string using a pre-defined template. The resulting HTML is written into a buffer provided by the caller.
void render_http_response_html(char *, int, char *, char *);

// Generates an HTTP response string
char *generate_http_response(int, char *, char *, int, int, char *, struct stat);

// processes a client request by parsing and interpreting the request path, setting appropriate HTTP response codes and paths,
// and returning the contents of the requested file or a directory listing as an HTTP response.
int process_client_request(void *);

char *get_mime_type(char *name) {
    // Extract the file extension from the file name using the `strrchr` function.
    char *ext = strrchr(name, '.');

    // If the extension is NULL (i.e., there is no file extension), return NULL.
    if (!ext) return NULL;

    // Check the extension against a list of known MIME types and return the corresponding MIME type if a match is found.
    if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0) return "text/html";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(ext, ".gif") == 0) return "image/gif";
    if (strcmp(ext, ".png") == 0) return "image/png";
    if (strcmp(ext, ".css") == 0) return "text/css";
    if (strcmp(ext, ".au") == 0) return "audio/basic";
    if (strcmp(ext, ".wav") == 0) return "audio/wav";
    if (strcmp(ext, ".avi") == 0) return "video/x-msvideo";
    if (strcmp(ext, ".mpeg") == 0 || strcmp(ext, ".mpg") == 0) return "video/mpeg";
    if (strcmp(ext, ".mp3") == 0) return "audio/mpeg";

    // If no match is found, return NULL.
    return NULL;
}

void wrong_command_usage(char *usage[], int argc) {
    // Create an empty string to hold the usage message.
    char usage_message[MAX_REQUEST_LENGTH] = "";
    int len = 0;

    // Loop through each argument in the `usage` array and append it to the usage message.
    for (int i = 1; i < argc; i++) {
        len += snprintf(usage_message + len, sizeof(usage_message) - len, "%s ", usage[i]);
    }

    // Print the usage message to stdout along with an error message and exit the program.
    fprintf(stdout, "Usage: server %s\n", usage_message);
    exit(EXIT_SUCCESS);
}

void set_http_response(int http_status_code, char **http_status_reason, char **http_response_body) {
    // Use a switch statement to set the reason phrase and response body based on the HTTP status code.
    switch (http_status_code) {
        case 302:
            *http_status_reason = "Found";
            *http_response_body = FOUND;
            break;
        case 400:
            *http_status_reason = "Bad Request";
            *http_response_body = BAD_REQUEST;
            break;
        case 403:
            *http_status_reason = "Forbidden";
            *http_response_body = FORBIDDEN;
            break;
        case 404:
            *http_status_reason = "Not Found";
            *http_response_body = NOT_FOUND;
            break;
        case 500:
            *http_status_reason = "Internal Server Error";
            *http_response_body = INTERNAL_SERVER_ERROR;
            break;
        default:
            *http_status_reason = "Not supported";
            *http_response_body = NOT_SUPPORTED;
            break;
    }
}

void parse_and_set_http_response(char *request_path, int *http_status_code, char **http_status_reason,
                                 char **http_response_body, char *file_path) {
    // set pointers to the beginning of the request path
    char *p, *q;
    p = request_path;
    // find the first space character in the request path
    q = strchr(p, ' ');

    // if no space character is found, the request path is invalid
    if (q == NULL) {
        fprintf(stderr, "Failed to parse request_path: %s\n", request_path);
        *http_status_code = 400;
        set_http_response(400, http_status_reason, http_response_body);
        return;
    }

    // replace the space character with a null terminator
    *q = '\0';

    // if the request method is not GET, return an error
    if (strcmp(p, "GET") != 0) {
        *http_status_code = 501;
        set_http_response(501, http_status_reason, http_response_body);
        return;
    }

    // move the pointer to the next character after the space
    p = q + 1;

    // find the next space character in the request path
    q = strchr(p, ' ');

    // if no space character is found, the request path is invalid
    if (q == NULL) {
        fprintf(stderr, "Failed to parse request_path: %s\n", request_path);
        *http_status_code = 400;
        set_http_response(400, http_status_reason, http_response_body);
        return;
    }

    // replace the space character with a null terminator and copy the file path to the given buffer
    *q = '\0';
    strncpy(file_path, p, strlen(p) + 1);

    // move the pointer to the next character after the space
    p = q + 1;

    // if the request version is not HTTP/1.0 or HTTP/1.1, return an error
    if (strncmp(p, "HTTP/1.0", 8) != 0 && strncmp(p, "HTTP/1.1", 8) != 0) {
        *http_status_code = 400;
        set_http_response(400, http_status_reason, http_response_body);
        return;
    }
}

void check_file_permissions(char *file_extension, char *root_path, struct stat *file_stat, int *http_status_code,
                            char **http_status_reason, char **http_response_body) {
    const char delimiter = '/'; // The delimiter character used to split the root path
    size_t root_path_len = strlen(root_path); // The length of the root path
    size_t file_ext_len = 0; // The length of the extracted file extension

    // Loop through the root path
    for (size_t i = 0; i < root_path_len; i++) {
        if (root_path[i] == delimiter) { // If the delimiter character is found
            file_extension[file_ext_len++] = delimiter; // Append the delimiter character to the file extension

            // Check if the file exists and can be accessed by the server
            if (stat(file_extension, file_stat) < 0) {
                *http_status_code = 404; // Set the HTTP status code to 404 (Not Found)
                set_http_response(*http_status_code, http_status_reason, http_response_body); // Set the HTTP response
                return;
            }

            // Check if the file is readable by the user, group, and others, or if a directory is executable by others
            if (((S_ISREG(file_stat->st_mode) && !(file_stat->st_mode & S_IRUSR)) ||
                 (S_ISREG(file_stat->st_mode) && !(file_stat->st_mode & S_IRGRP)) ||
                 (S_ISREG(file_stat->st_mode) && !(file_stat->st_mode & S_IROTH))) ||
                (S_ISDIR(file_stat->st_mode) && !(file_stat->st_mode & S_IXOTH))) {
                *http_status_code = 403; // Set the HTTP status code to 403 (Forbidden)
                set_http_response(*http_status_code, http_status_reason, http_response_body); // Set the HTTP response
                return;
            }
        } else {
            file_extension[file_ext_len++] = root_path[i]; // Append the character to the file extension
        }
    }

    // Check if the file exists and can be accessed by the server
    if (file_ext_len > 0) {
        if (stat(file_extension, file_stat) < 0) {
            *http_status_code = 404; // Set the HTTP status code to 404 (Not Found)
            set_http_response(*http_status_code, http_status_reason, http_response_body); // Set the HTTP response
            return;
        }

        // Check if the file is readable by the user, group, and others, or if a directory is executable by others
        if (((S_ISREG(file_stat->st_mode) && !(file_stat->st_mode & S_IRUSR)) ||
             (S_ISREG(file_stat->st_mode) && !(file_stat->st_mode & S_IRGRP)) ||
             (S_ISREG(file_stat->st_mode) && !(file_stat->st_mode & S_IROTH))) ||
            (S_ISDIR(file_stat->st_mode) && !(file_stat->st_mode & S_IXOTH))) {
            *http_status_code = 403; // Set the HTTP status code to 403 (Forbidden)
            set_http_response(*http_status_code, http_status_reason, http_response_body); // Set the HTTP response
            return;
        }
    }
}

char *
generate_directory_listing(char *index_file_path, size_t file_size, char *http_response_body, struct dirent *entry,
                           struct stat *file_stat, DIR *dir) {

    char time_buffer[128];
    size_t body_size = file_size + 1 + (RESPONSE_LENGTH);

    // allocate memory for 'http_response_body'
    http_response_body = (char *) malloc(body_size * sizeof(char));

    // check if memory allocation was successful
    if (http_response_body == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory\n");
        exit(EXIT_FAILURE);
    }

    // initialize 'http_response_body' with 0
    memset(http_response_body, 0, body_size);

    // write the header of the directory listing to 'http_response_body'
    int offset = snprintf(http_response_body, body_size, DIRECTORY_HEADER, index_file_path, index_file_path);

    // open the directory stream for the specified directory
    dir = opendir(index_file_path);
    // iterate through each file in the directory
    while ((entry = readdir(dir)) != NULL) {
        // allocate memory for the path to the current file
        char *current_file_path = (char *) malloc(strlen(index_file_path) + 1 + strlen(entry->d_name) + 1);

        // check if memory allocation was successful
        if (current_file_path == NULL) {
            fprintf(stderr, "Error: Failed to allocate memory\n");
            exit(EXIT_FAILURE);
        }

        // initialize 'current_file_path' with 0
        memset(current_file_path, 0, strlen(index_file_path) + 1 + strlen(entry->d_name) + 1);

        // concatenate the path to the current file to 'current_file_path'
        snprintf(current_file_path, strlen(index_file_path) + 1 + strlen(entry->d_name), "%s%s", index_file_path,
                 entry->d_name);

        // get the file status of the current file
        stat(current_file_path, file_stat);

        // write the file name and link to 'http_response_body'
        offset += snprintf(http_response_body + offset, body_size - offset, CREATE_TABLE, entry->d_name,
                           entry->d_name);

        // format the last modified time of the current file in RFC1123 format
        strftime(time_buffer, sizeof(time_buffer), RFC1123FMT, localtime(&file_stat->st_mtime));

        // write the last modified time of the current file to 'http_response_body'
        offset += snprintf(http_response_body + offset, body_size - offset, "%s</td>\r\n<td>", time_buffer);

        // write the file size of the current file to 'http_response_body' if it's not a directory
        if (!S_ISDIR(file_stat->st_mode)) {
            offset += snprintf(http_response_body + offset, body_size - offset, "%ld", file_stat->st_size);
        }

        // write the end of the table row to 'http_response_body'
        offset += snprintf(http_response_body + offset, body_size - offset, "</td></tr>\r\n\r\n");

        // free memory allocated for 'current_file_path'
        free(current_file_path);
    }
    // write the end of the HTML page to http_response_body
    snprintf(http_response_body + offset, body_size - offset, END_OF_PAGE);

    // close the directory stream
    closedir(dir);

    // return the dynamically allocated string containing the directory listing
    return http_response_body;

}

void render_http_response_html(char *html, int http_status_code, char *http_status_reason, char *http_response_body) {
    // Fill in the HTML_TEMPLATE string with the provided status code, status reason, and response body
    sprintf(html, HTML_TEMPLATE, http_status_code, http_status_reason, http_status_code, http_status_reason,
            http_response_body);
}

char *generate_http_response(int http_status_code, char *http_status_reason, char *file_path,
                             int is_file, int file_size, char *html, struct stat index_file_stat) {

    time_t now;
    char time_buffer[128];
    now = time(NULL);

    // Buffer to hold the entire HTTP response
    char buffer[MAX_REQUEST_LENGTH + sizeof(time_buffer) + 1000] = {0};
    char *ptr = buffer; // Pointer to the current position in the buffer
    size_t length;
    int len;

    // Write the HTTP header to the buffer
    length = snprintf(ptr, sizeof(buffer), RESPONSE_HEADER, http_status_code, http_status_reason);
    ptr += length;

    // Write the current date and time to the buffer
    strftime(ptr, sizeof(time_buffer), RFC1123FMT, localtime(&now));
    ptr += strlen(ptr);

    // If the status code is 302 (redirect), add the Location header to the buffer
    if (http_status_code == 302) {
        length = snprintf(ptr, sizeof(buffer) - (ptr - buffer), LOCATION, file_path);
        ptr += length;
    }

    // Add the Content-Type header to the buffer
    if (is_file == false) {
        length = snprintf(ptr, sizeof(buffer) - (ptr - buffer), CONTENT_TYPE, TYPE);
        ptr += length;
    } else {
        length = snprintf(ptr, sizeof(buffer) - (ptr - buffer), CONTENT_TYPE, get_mime_type(file_path));
        ptr += length;
    }

    // If the status code is not 200 (success), add the Content-Length header and the HTML response body
    if (http_status_code != 200) {
        len = (int) strlen(html);
        length = snprintf(ptr, sizeof(buffer) - (ptr - buffer), CONTENT_LENGTH, len);
        ptr += length;
    }
        // If the requested resource is a file, add the Content-Length header and the Last-Modified header
    else {
        length = snprintf(ptr, sizeof(buffer) - (ptr - buffer), CONTENT_LENGTH, file_size);
        ptr += length;
        strftime(ptr, sizeof(time_buffer), TIME, localtime(&index_file_stat.st_mtime));
        ptr += strlen(ptr);
    }

    // Add the Connection: close header to the buffer
    length = snprintf(ptr, sizeof(buffer) - (ptr - buffer), CLOSE_CONNECTION);
    ptr += length;

    // If the status code is not 200, add the HTML response body to the buffer
    if (http_status_code != 200)
        snprintf(ptr, sizeof(buffer) - (ptr - buffer), "%s", html);

    // Duplicate the buffer and return the duplicate
    return strdup(buffer);
}

int process_client_request(void *file_des) {
    // Cast the void pointer as an integer to get the client socket file descriptor.
    int client_socket = *(int *) file_des;

    // Initialize variables for HTTP status code, file size, and file descriptor.
    int http_status_code = 0;
    int file_size = 0;
    int file_descriptor;

    // Initialize a boolean to keep track of whether the requested resource is a file.
    bool is_file = false;

    // Initialize character arrays for the request path, file path, root path, index file path, and HTML response.
    char request_path[MAX_REQUEST_LENGTH] = {0};
    char file_path[MAX_REQUEST_LENGTH] = {0};
    char root_path[PATH_LENGTH] = {0};
    char index_file_path[PATH_LENGTH] = {0};
    char html[HTML_LENGTH] = {0};

    // Initialize pointers to character arrays for the HTTP status reason, HTTP response body, file extension,
    // and directory path, and the full HTTP response.
    char *http_status_reason = NULL;
    char *http_response_body = NULL;
    char *file_extension = NULL;
    char *directory_path = NULL;
    char *http_response = NULL;

    // Initialize structures for file and index file stats, and a directory entry.
    struct stat file_stat = {0};
    struct stat index_file_stat = {0};
    struct dirent *entry = NULL;
    DIR *dir = NULL;

    // If the client socket is less than zero, there was an error accepting the connection.
    if (client_socket < 0) {
        perror("error: accept\n");
        http_status_code = 500;
        set_http_response(500, &http_status_reason, &http_response_body);
    }

    // Read the request path from the client socket. If there was an error, set the HTTP status code to 500.
    if ((int) read(client_socket, request_path, sizeof(request_path)) < 0) {
        perror("error: read\n");
        http_status_code = 500;
        set_http_response(500, &http_status_reason, &http_response_body);
    }

    // Parse the request path and set the appropriate HTTP response codes and paths.
    parse_and_set_http_response(request_path, &http_status_code, &http_status_reason, &http_response_body, file_path);

    // If the HTTP status code is still 0, continue processing the request.
    if (http_status_code == 0) {
        // Set the root path and index file path using the requested file path.
        snprintf(root_path, PATH_LENGTH, ".%s", file_path);
        snprintf(index_file_path, PATH_LENGTH, ".%s", file_path);

        // Open the root path as a directory. If it's null, the requested resource is a file.
        dir = opendir(root_path);

        // Check if the directory is null
        if (dir == NULL) {
            // Allocate memory for file extension
            file_extension = (char *) malloc((strlen(root_path) + 1) * sizeof(char));
            // Check if memory allocation failed
            if (file_extension == NULL) {
                fprintf(stderr, "Error: Failed to allocate memory\n");
                exit(EXIT_FAILURE);
            }

            // Set all values in file_extension to 0
            memset(file_extension, 0, (strlen(root_path) + 1) * sizeof(char));

            // Check file permissions for the root path and set HTTP response status and body accordingly
            check_file_permissions(file_extension, root_path, &file_stat, &http_status_code, &http_status_reason, &
                    http_response_body);

            // Free the memory allocated for file_extension
            free(file_extension);

            // If HTTP status code is 0, file exists, and set appropriate response values
            if (http_status_code == 0) {
                // Open the file at root_path in read-only mode and get its stats
                file_descriptor = open(root_path, O_RDONLY);
                stat(root_path, &index_file_stat);

                // Set HTTP status code to 200, HTTP status reason to "OK", and is_file to true
                http_status_code = 200;
                http_status_reason = "OK";
                is_file = true;

                // Set file_size to the size of the file
                file_size = (int) index_file_stat.st_size;

                // Allocate memory for http_response_body
                http_response_body = (char *) malloc((file_size + 1 + RESPONSE_LENGTH) * sizeof(char));

                // Check if memory allocation failed
                if (http_response_body == NULL) {
                    fprintf(stderr, "Error: Failed to allocate memory\n");
                    exit(EXIT_FAILURE);
                }

                // Set all values in http_response_body to 0
                memset(http_response_body, 0, (file_size + 1 + RESPONSE_LENGTH) * sizeof(char));

                // Read the contents of the file into http_response_body
                if (read(file_descriptor, http_response_body, file_size) < 0) {
                    // If an error occurs, set HTTP status code to 500 and appropriate response values
                    perror("error: read\n");
                    http_status_code = 500;
                    set_http_response(500, &http_status_reason, &http_response_body);
                }
            }
        }
            // If the URL path corresponds to a directory in the server's file system, a directory listing is generated and returned in the response.
        else {
            // Check if the file path ends with a forward slash.
            if (file_path[strlen(file_path) - 1] != '/') {
                // If not, redirect to the same path with a forward slash appended.
                http_status_code = 302;
                set_http_response(302, &http_status_reason, &http_response_body);
                closedir(dir);
            } else {
                // Append the server's root directory to the file path.
                strcat(root_path, ROOT_DIR);
                // Attempt to open the file at the modified path.
                file_descriptor = open(root_path, O_RDONLY);
                if (file_descriptor > 0) {
                    // If the file was successfully opened, read its content into the response body.
                    stat(root_path, &index_file_stat);
                    http_status_code = 200;
                    http_status_reason = OK;

                    file_size = (int) index_file_stat.st_size;
                    // Allocate memory for the response body.
                    http_response_body = (char *) malloc((file_size + 1 + RESPONSE_LENGTH) * sizeof(char));
                    if (http_response_body == NULL) {
                        // If memory allocation failed, exit with an error message.
                        fprintf(stderr, "Error: Failed to allocate memory\n");
                        exit(EXIT_FAILURE);
                    }

                    // Initialize the response body with null characters.
                    memset(http_response_body, 0, (file_size + 1 + RESPONSE_LENGTH) * sizeof(char));
                    if (read(file_descriptor, http_response_body, file_size) < 0) {
                        // If reading the file content failed, set the response code to 500 (Internal Server Error).
                        perror("error: read\n");
                        http_status_code = 500;
                        set_http_response(500, &http_status_reason, &http_response_body);
                    }
                    closedir(dir);
                } else {
                    // If the modified path doesn't correspond to a file, assume it corresponds to a directory.
                    stat(index_file_path, &index_file_stat);
                    http_status_code = 200;
                    http_status_reason = "OK";

                    file_size = 0;
                    while ((entry = readdir(dir)) != NULL) {
                        // Iterate over the directory's entries to determine the total size of its contents.
                        directory_path = (char *) malloc(
                                (strlen(index_file_path) + 1 + strlen(entry->d_name)) * sizeof(char));
                        if (directory_path == NULL) {
                            // If memory allocation failed, exit with an error message.
                            fprintf(stderr, "Error: Failed to allocate memory for directory_path.\n");
                            exit(EXIT_FAILURE);
                        }

                        // Construct the path to the directory entry.
                        memset(directory_path, 0, (strlen(index_file_path) + 1 + strlen(entry->d_name)) * sizeof(char));
                        strcat(directory_path, index_file_path);
                        strcat(directory_path, entry->d_name);

                        // Determine the size of the directory entry's content.
                        stat(directory_path, &file_stat);
                        file_size += (int) (file_stat.st_size);

                        free(directory_path);
                    }
                    // Generate the directory listing and set the response body to its contents.
                    http_response_body = generate_directory_listing(index_file_path, file_size, http_response_body,
                                                                    entry, &file_stat, dir);
                }
            }
        }
    }

    // If the HTTP status code is not 200, render an HTML response with the status code, reason, and body
    if (http_status_code != 200) {
        render_http_response_html(html, http_status_code, http_status_reason, http_response_body);
    }

    // Generate the HTTP response based on the given parameters
    http_response = generate_http_response(http_status_code, http_status_reason, file_path, is_file, file_size, html,
                                           index_file_stat);

    // Write the HTTP response to the client socket
    if (write(client_socket, http_response, strlen(http_response)) < 0) {
        perror("error: read\n");
        free(http_response);
        close(client_socket);
        return EXIT_FAILURE;
    }

    // If the HTTP status code is 200, write the response body to the client socket
    if (http_status_code == 200) {
        if (write(client_socket, http_response_body, file_size) < 0) {
            perror("error: read\n");
            free(http_response_body);
            free(http_response);
            close(client_socket);
            return EXIT_FAILURE;
        }
        free(http_response_body);
    }

    // Free allocated memory and close the client socket
    free(http_response);
    close(client_socket);

    // Return success or failure based on whether there were any errors
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {

    // Declare variables to store command line arguments and other required values
    char *end_ptr;
    long port_args, pool_size_args, max_request_args;
    int *req_ptr;
    int fd, new;
    struct sockaddr_in server_address;
    int max_concurrent_requests;
    int *request_fds;

    // Check if the user provided the correct number of command line arguments
    if (argc != 4) {
        // If not, print the usage message and exit with failure status
        wrong_command_usage(argv, argc);
        exit(EXIT_FAILURE);
    }

    // Convert and validate the first command line argument (port number)
    port_args = strtol(argv[1], &end_ptr, 10);
    if (*end_ptr != '\0' || port_args < 0 || port_args > 65535) {
        // If conversion or validation fails, print the usage message and exit with failure status
        wrong_command_usage(argv, argc);
    }

    // Convert and validate the second command line argument (thread pool size)
    pool_size_args = strtol(argv[2], &end_ptr, 10);
    if (*end_ptr != '\0' || pool_size_args < 1) {
        // If conversion or validation fails, print the usage message and exit with failure status
        wrong_command_usage(argv, argc);
    }

    // Convert and validate the third command line argument (maximum number of concurrent requests)
    max_request_args = strtol(argv[3], &end_ptr, 10);
    if (*end_ptr != '\0' || max_request_args < 1) {
        // If conversion or validation fails, print the usage message and exit with failure status
        wrong_command_usage(argv, argc);
    }

    // Cast the maximum number of concurrent requests to an integer and create a thread pool with the given size
    max_concurrent_requests = (int) max_request_args;
    threadpool *pool = create_threadpool((int) pool_size_args);
    if (pool == NULL) {
        // If thread pool creation fails, print an error message and exit with failure status
        perror("pool");
        exit(EXIT_FAILURE);
    }

    // Create a TCP socket and check for errors
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        // If socket creation fails, print an error message, destroy the thread pool, and exit with failure status
        perror("error: socket\n");
        destroy_threadpool(pool);
        exit(EXIT_FAILURE);
    }

    // Set up the server address
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons((int) port_args);
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind the socket to the server address and check for errors
    if (bind(fd, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
        // If binding fails, print an error message, destroy the thread pool, and exit with failure status
        perror("error: bind\n");
        destroy_threadpool(pool);
        exit(EXIT_FAILURE);
    }

    // allocate memory for an array of file descriptors that correspond to incoming client requests
    request_fds = (int *) malloc(max_concurrent_requests * sizeof(int));
    if (request_fds == NULL) {
        // handle failure to allocate memory
        fprintf(stderr, "Error: Failed to allocate memory for request_fds.\n");
        destroy_threadpool(pool);
        exit(EXIT_FAILURE);
    }

    // mark the socket referred to by `fd` as a passive socket and start listening for incoming connections
    if (listen(fd, 5) < 0) {
        // handle errors when setting up the passive socket
        perror("error: listen\n");
        destroy_threadpool(pool);
        close(fd);
        exit(EXIT_FAILURE);
    }

    // set the pointer `req_ptr` to the beginning of the array `request_fds`
    req_ptr = request_fds;
    do {
        // block until an incoming connection arrives and create a new socket for the connection
        new = accept(fd, NULL, NULL);
        if (new < 0) {
            // handle errors when accepting incoming connections
            perror("error: accept\n");
            continue;
        }
        // add the new socket to the array of file descriptors and dispatch a thread to process the client request
        *req_ptr = new;
        dispatch(pool, process_client_request, req_ptr);
        req_ptr++;
    } while (req_ptr < request_fds + max_concurrent_requests);

    // close the passive socket and destroy the thread pool
    close(fd);
    destroy_threadpool(pool);
    // free the memory allocated for the array of file descriptors
    free(request_fds);

    // return with success
    return EXIT_SUCCESS;
}