#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include "http.h"

#define debug 0
#define BUFSIZE 512

const char *get_mime_type(const char *file_extension) {
    if (strcmp(".txt", file_extension) == 0) {
        if(debug) printf("txt\n");
        return "text/plain";
    } else if (strcmp(".html", file_extension) == 0) {
        if(debug) printf("html\n");
        return "text/html";
    } else if (strcmp(".jpg", file_extension) == 0) {
        if(debug) printf("jpg\n");
        return "image/jpeg";
    } else if (strcmp(".png", file_extension) == 0) {
        if(debug) printf("png\n");
        return "image/png";
    } else if (strcmp(".pdf", file_extension) == 0) {
        if(debug) printf("pdf\n");
        return "application/pdf";
    }

    return NULL;
}


int write_http_response(int fd, const char *resource_path) {
    // Pre-canned 404 error.
    char error404[] = "HTTP/1.0 404 Not Found\r\n";
    if(debug) printf("Resource path: %s\n", resource_path);

    // I never was able to get stat to work as anything but a "does the file exist" command on my machine. Thankfully, it serves that purpose quite well.
    struct stat mystatbuf; 
    int filelength = stat(resource_path, &mystatbuf);
    if (filelength == -1){
        if(debug) printf("File Not Found.\n");
        // If we can't find the file, write a 404 error.
        write(fd, error404, strlen(error404));
        return -1;
    }
    //Below is the response header

     
    const char *dot = strrchr(resource_path, '.');
    if(!dot || dot == resource_path){
        // Ditto as above
        write(fd, error404, strlen(error404));
        return -1;
    }


    // Make a buffer, initialize to zeroes.
    char buf[BUFSIZE];
    memset(buf, 0, sizeof(buf));
    // The rest of this code is used to construct the response.
    strcpy(buf, "HTTP/1.0 200 OK\r\n");
    strcat(buf, "Content-Type: ");

    if(debug) printf("Size from stat: %ld\n", mystatbuf.st_size);
    strcat(buf, get_mime_type(dot));
    strcat(buf, "\r\nContent-Length: ");
    // The buffer for the size of the content to return
    char sizebuf[12];

    // Open the file at the destination, seek to the end and find position to get the file length.
    FILE *myFile = fopen(resource_path, "rb");
    int bytes_read;
    fseek(myFile, 0, SEEK_END);
    // Print the result into the size buffer, then concatenate it to the current request
    sprintf(sizebuf, "%ld", ftell(myFile));
    if(debug) printf("Size buffer: %ld\n", ftell(myFile));
    strcat(buf, sizebuf);
    strcat(buf, "\r\n\r\n");
    if(debug) printf("Request:\n%s\n", buf);
    // Write the response header into the socket
    write(fd, buf, strlen(buf));
    // Go back to the head of the file to keep looking through
    fseek(myFile, 0, SEEK_SET);

    // Write the response and content into the socket
    while((bytes_read = fread(buf, 1, BUFSIZE, myFile)) > 0){
        if (write(fd, buf, bytes_read) == -1) {
            // In case of error, close the fd and return an error
            perror("write");
            close(fd);
            return -1;
        }
    }
    if (bytes_read == -1) {
        perror("read");
        return -1;
    }


    return 0;
}

int read_http_request(int fd, char *resource_name) {
    // Make a buffer.
    char buffer[BUFSIZE];
    // Read from the fd argument into the buffer.
    read(fd, buffer, sizeof(buffer));
    // These next two lines are kinda cheating, but basically cut out the pertinent parts of the GET request using strchr.
    char *tempbuf = strchr(buffer, '/');
    char *eventemperbuf = strchr(tempbuf, ' ');
    // Terminate the original string by using a pointer, which you then set to the null character. Probably not totally safe, but perfectly functional for our limited purposes.
    eventemperbuf[0] = '\0';
    if(debug) printf("Pass Buffer: \"%s\"\n", tempbuf);
    // Strcpy into the resource_name argument, starting one character forward to drop an unnecessary slash.
    strcpy(resource_name, tempbuf+1);

    return 0;
}

