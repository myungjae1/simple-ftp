#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include "../util/socket_util.h"
#include "../util/string_util.h"
#include "../util/constant.h"

int create_socket(unsigned int server_ip, unsigned int server_command_port);

int wait_for_response(int* server_command_socket, int response_code, char* server_response);

int main(int argc, char *argv[])
{
  if(argc != 3)
  {
    printf("Usage: ftp_client [server_ip] [server_port]\n");
    exit(0);
  }

  // for command channel
  unsigned int server_ip = ntohl(inet_addr(argv[1]));
  unsigned int server_command_port = atoi(argv[2]);

  // for data channel (passive mode)
  unsigned int server_data_ip;
  unsigned int server_data_port;

  int server_command_socket = create_socket(server_ip, server_command_port);

  printf("\r\nCommand-Socket connected.\r\n");
  printf("Server IP: %s\r\n", argv[1]);
  printf("Server command-port: %d\r\n", server_command_port);
  printf("----------------\r\n");

  char user_input[MAX_BUF_SIZE];
  char user_input_command[10];
  char user_input_parameter[MAX_BUF_SIZE];
  char message_to_send[MAX_BUF_SIZE];
  char server_response[MAX_BUF_SIZE];

  // wait for welcome message
  wait_for_response(&server_command_socket, 220, server_response);

  while(1)
  {
    memset(user_input, 0, sizeof(user_input));
    memset(message_to_send, 0, sizeof(message_to_send));
    memset(server_response, 0, sizeof(server_response));

    printf("myftp> ");
    fgets(user_input, MAX_BUF_SIZE, stdin);

    get_token(user_input_command, user_input, 0);
    get_token(user_input_parameter, user_input, 1);

    snprintf(message_to_send, MAX_BUF_SIZE, "%s %s\r\n", user_input_command, user_input_parameter);

    if(strcmp("USER", user_input_command) == 0)
    {
      send_to_socket(&server_command_socket, message_to_send);
      wait_for_response(&server_command_socket, 331, server_response);
    }
    else if(strcmp("PASS", user_input_command) == 0)
    {
      send_to_socket(&server_command_socket, message_to_send);
      wait_for_response(&server_command_socket, 230, server_response);
    }
    else if(strcmp("PWD", user_input_command) == 0)
    {
      send_to_socket(&server_command_socket, message_to_send);
      wait_for_response(&server_command_socket, 257, server_response);
    }
    else if(strcmp("CWD", user_input_command) == 0)
    {
      send_to_socket(&server_command_socket, message_to_send);
      wait_for_response(&server_command_socket, 250, server_response);
    }
    else if(strcmp("CDUP", user_input_command) == 0)
    {
      send_to_socket(&server_command_socket, message_to_send);
      wait_for_response(&server_command_socket, 250, server_response);
    }
    else if(strcmp("PASV", user_input_command) == 0)
    {
      send_to_socket(&server_command_socket, message_to_send);
      wait_for_response(&server_command_socket, 227, server_response);

      // parse the ip & port of the server data channel
      parse_server_data_channel_info(server_response, &server_data_ip, &server_data_port);
    }
    else if(strcmp("LIST", user_input_command) == 0)
    {
      send_to_socket(&server_command_socket, message_to_send);
      wait_for_response(&server_command_socket, 150, server_response);

      int server_data_socket = create_socket(server_data_ip, server_data_port);
      receive_file(&server_data_socket, stdout);
      close_socket(&server_data_socket);

      wait_for_response(&server_command_socket, 226, server_response);
    }
    else if(strcmp("RETR", user_input_command) == 0
         || strcmp("REVRETR", user_input_command) == 0)
    {
      send_to_socket(&server_command_socket, message_to_send);
      wait_for_response(&server_command_socket, 150, server_response);

      FILE* file = fopen(user_input_parameter, "wb");

      int server_data_socket = create_socket(server_data_ip, server_data_port);
      receive_file(&server_data_socket, file);
      close_socket(&server_data_socket);

      fclose(file);

      wait_for_response(&server_command_socket, 226, server_response);
    }
    else if(strcmp("STOR", user_input_command) == 0
         || strcmp("REVSTOR", user_input_command) == 0)
    {
      send_to_socket(&server_command_socket, message_to_send);
      wait_for_response(&server_command_socket, 150, server_response);

      // if the command is 'REVSTOR', send the file in reverse order
      bool reverse = false;
      if(strcmp("REVSTOR", user_input_command) == 0)
      {
        reverse = true;
      }

      int server_data_socket = create_socket(server_data_ip, server_data_port);
      send_file(&server_data_socket, user_input_parameter, reverse);
      close_socket(&server_data_socket);

      wait_for_response(&server_command_socket, 226, server_response);
    }
    else
    {
      printf("Not Implemented.\r\n");
    }
  }

  return 0;
}

int create_socket(unsigned int server_ip, unsigned int server_command_port)
{
  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(server_ip);
  server_addr.sin_port = htons(server_command_port);

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if(sockfd < 0)
  {
    printf("socket create error");
    exit(1);
  }

  if(connect(sockfd, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0)
  {
    printf("socket connect error");
    exit(1);
  }

  return sockfd;
}

int wait_for_response(int* server_command_socket, int response_code, char* server_response)
{
  char server_response_buffer[MAX_BUF_SIZE];
  char server_response_code[MAX_BUF_SIZE];

  memset(server_response_buffer, 0, sizeof(server_response_buffer));
  memset(server_response_code, 0, sizeof(server_response_code));
  
  while(receive_from_socket(server_command_socket, server_response_buffer) > 0)
  {
    printf("Server response: %s\r\n", server_response_buffer);

    get_token(server_response_code, server_response_buffer, 0);
    if(response_code == atoi(server_response_code))
    {
      strncpy(server_response, server_response_buffer, MAX_BUF_SIZE);
      return 0;
    }
    else if(atoi(server_response_code) >= 500)
    {
      printf("Server error occured.\r\n");
      return -1;
    
    }
  }

  return -1;
}
