#include "socket_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int close_socket(int* sockfd)
{
    if (*sockfd < 0)
    {
      return 0;
    }

    int result = close(*sockfd);
    *sockfd = -1;
    return result;
}

int send_to_socket(int* sockfd, char* message)
{
  write(*sockfd, message, strlen(message));
  return 0;
}

int receive_from_socket(int* sockfd, char* buffer)
{
  memset(buffer, 0, strlen(buffer));
  int read_count = read(*sockfd, buffer, MAX_BUF_SIZE);
  return read_count;
}

int send_file(int* data_sockfd, char* file_name, bool reverse)
{
  FILE* file = fopen(file_name, "rb");
  if(file)
  {
    if(reverse)
    {
      // get the size of file
      fseek(file, 0, SEEK_END);
      long file_size = ftell(file);

      // go to the front of file after getting the size of file
      fseek(file, 0, SEEK_SET); 

      // read file contents
      char* buffer = malloc(file_size * sizeof(char));
      fread(buffer, sizeof(char), file_size, file);

      // reverse file contents
      int mirror_index;
      char temp;
      long i;
      for(i = 0 ; i < file_size/2 ; i++)
      {
        mirror_index = (file_size - 1) - i;

        temp = buffer[i];
        buffer[i] = buffer[mirror_index];
        buffer[mirror_index] = temp;
      }

      // send reversed file
      write(*data_sockfd, buffer, file_size);

      free(buffer);
    }
    else
    {
      char buffer[MAX_BUF_SIZE];
      int read_count = 1;

      fseek(file, 0, SEEK_SET);
      while ((read_count = fread(buffer, 1, MAX_BUF_SIZE, file)) > 0)
      {
        write(*data_sockfd, buffer, read_count);
      }
    }
  }

  return fclose(file);
}

int receive_file(int* data_sockfd, FILE* file)
{
  if(file)
  {
    char buffer[MAX_BUF_SIZE];
    int read_count = 1;

    fseek(file, 0, SEEK_SET);
    while((read_count = read(*data_sockfd, buffer, MAX_BUF_SIZE)) > 0)
    {
      fwrite(buffer, 1, read_count, file);
    }

    return 0;
  }
  
  return -1;
}
