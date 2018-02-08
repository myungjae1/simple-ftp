#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "../util/socket_util.h"
#include "../util/string_util.h"
#include "../util/constant.h"

char server_response_150[] = "150 About to open data connection.\r\n";
char server_response_200[] = "200 Type set to I.\r\n";
char server_response_202[] = "202 Command not implemented.\r\n";
char server_response_215[] = "215 UNIX\r\n";
char server_response_220[] = "220 Welcome\r\n";
char server_response_226[] = "226 Transfer completed.\r\n";
char server_response_227[] = "227 Entering Passive Mode (%ld,%ld,%ld,%ld,%d,%d).\r\n";
char server_response_230[] = "230 User logged in, proceed. Logged out if appropriate.\r\n";
char server_response_250[] = "250 Requested action completed.\r\n";
char server_response_257[] = "257 \"%s\" created.\r\n";
char server_response_331[] = "331 User name okay, need password.\r\n";
char server_response_451[] = "451 Requested action aborted. Local error in processing.\r\n";
char server_response_500[] = "500 Error.\r\n";

int listen_sock;
int client_sock;
int data_transfer_sock;

char msg_buf[MAX_BUF_SIZE];
char current_path[MAX_BUF_SIZE];

void quit();
void handle_client(int);
int login(int);

int handle_PASV(int);
int handle_LIST(int);
int handle_RETR(int*, bool);
int handle_STOR(int*);

int main(int argc, char *argv[])
{
  srand(time(NULL));
  signal(SIGINT, quit);

  int listen_port = PORT_NUM;
  if(argc == 2)
  {
    listen_port = atoi(argv[1]);
  }

  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(listen_port);

  if((listen_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    printf("socket create error\n");
    exit(1);
  }

  if(bind(listen_sock, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0)
  {
    printf("socket bind error\n");
    exit(1);
  }

  if(listen(listen_sock, 5) < 0)
  {
    printf("socket listen error\n");
    exit(1);
  }

  while(1)
  {
    struct sockaddr_in client_addr;
    unsigned int client_addr_size = sizeof(client_addr);
    if((client_sock = accept(listen_sock, (struct sockaddr*) &client_addr, &client_addr_size)) < 0)
    {
      printf("socket accept error\n");
      exit(1);
    }

    // to handle multiple clients
    int pid = fork();
    if(pid > 0) // main process
    {
      close_socket(&client_sock);
    }
    else if(pid == 0) // child process
    {
      close_socket(&listen_sock);
      handle_client(client_sock);
    }
    else // fork error
    {
      printf("fork error");
      exit(1);
    }

  }

  quit();
  return 0;
}

void handle_client(int sockfd)
{
  // send welcome message
  send_to_socket(&sockfd, server_response_220);
     
  // login
  login(sockfd);

  while(1)
  {
    receive_from_socket(&sockfd, msg_buf);

    printf("%s\n", msg_buf);
    if(strncmp("SYST", msg_buf, 4) == 0)
    {
      send_to_socket(&sockfd, server_response_215);
    }
    else if(strncmp("PWD", msg_buf, 3) == 0)
    {
      getcwd(current_path, sizeof(current_path));

      char response_message[MAX_BUF_SIZE];
      memset(response_message, 0, sizeof(response_message));
      snprintf(response_message, MAX_BUF_SIZE, server_response_257, current_path);
      send_to_socket(&sockfd, response_message);
    }
    else if(strncmp("TYPE", msg_buf, 4) == 0)
    {
      send_to_socket(&sockfd, server_response_200);
    }
    else if(strncmp("PASV", msg_buf, 4) == 0)
    {
      handle_PASV(sockfd);
    }
    else if(strncmp("LIST", msg_buf, 4) == 0)
    {
      handle_LIST(sockfd);
    }
    else if(strncmp("CDUP", msg_buf, 4) == 0)
    {
      if (chdir("..") == 0)
      {
        send_to_socket(&sockfd, server_response_250);
      }
      else
      {
        send_to_socket(&sockfd, server_response_500);
      }
    }
    else if(strncmp("CWD", msg_buf, 3) == 0)
    {
      char path[MAX_BUF_SIZE];
      get_token(path, msg_buf, 1);
      
      if (chdir(path) == 0)
      {
        send_to_socket(&sockfd, server_response_250);
      }
      else
      {
        send_to_socket(&sockfd, server_response_500);
      }

    }
    else if(strncmp("RETR", msg_buf, 4) == 0
         || strncmp("REVRETR", msg_buf, 7) == 0)
    {
      // if the command is 'REVRETR', send the file in reverse order
      bool reverse = false;
      if(strncmp("REVRETR", msg_buf, 7) == 0)
      {
        reverse = true;
      }

      handle_RETR(&sockfd, reverse);
    }
    else if(strncmp("STOR", msg_buf, 4) == 0
         || strncmp("REVSTOR", msg_buf, 7) == 0)
    {
      handle_STOR(&sockfd);
    }
    else
    {
      send_to_socket(&sockfd, server_response_202);
    }
  }
}

int login(int sockfd)
{
  // username
  while(1)
  {
    receive_from_socket(&sockfd, msg_buf);
    printf("%s\n", msg_buf);

    if(strncmp("USER ", msg_buf, 5) == 0)
    {
      send_to_socket(&sockfd, server_response_331);
      break;
    }
    else 
    {
      send_to_socket(&sockfd, server_response_202);
    }
  }

  // password
  while(1)
  {
    receive_from_socket(&sockfd, msg_buf);
    printf("%s\n", msg_buf);

    if(strncmp("PASS ", msg_buf, 5) == 0)
    {
      send_to_socket(&sockfd, server_response_230);
      break;
    }
    else 
    {
      send_to_socket(&sockfd, server_response_202);
    }
  }

  return 0;
}

int handle_PASV(int sockfd)
{
  if((data_transfer_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    printf("data transfer socket create error\n");
    exit(1);
  }

  struct sockaddr_in data_transfer_addr;
  memset(&data_transfer_addr, 0, sizeof(data_transfer_addr));
  data_transfer_addr.sin_family = AF_INET;
  data_transfer_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  // until finding a port which is not being used
  int data_transfer_port;
  while(1)
  {
    data_transfer_port = (rand() % 40000) + 1024; // between 1024 ~ 39999
    data_transfer_addr.sin_port = htons(data_transfer_port);

    if(bind(data_transfer_sock, (struct sockaddr*) &data_transfer_addr, sizeof(struct sockaddr_in)) >= 0)
    {
      break;
    }
  }

  printf("data transfer port number (random) : %d\r\n", data_transfer_port);

  if(listen(data_transfer_sock, 5) < 0)
  {
    printf("data transfer socket listen error\r\n");
    close_socket(&data_transfer_sock);
    return 1;
  }

  struct sockaddr_in server_addr;
  unsigned int server_addr_length = sizeof(server_addr);
  getsockname(sockfd, (struct sockaddr*) &server_addr, &server_addr_length);

  long ip = inet_addr(inet_ntoa(server_addr.sin_addr));
  int data_transfer_port_upper_8bit = data_transfer_port / 256;
  int data_transfer_port_lower_8bit = data_transfer_port % 256;

  char response_message[MAX_BUF_SIZE];
  memset(response_message, 0, sizeof(response_message));
  snprintf(response_message, MAX_BUF_SIZE, server_response_227,
           ip&0xff, ip>>8&0xff, ip>>16&0xff, ip>>24&0xff,
           data_transfer_port_upper_8bit, data_transfer_port_lower_8bit);

  printf("response : %s\r\n", response_message);

  send_to_socket(&sockfd, response_message);
  return 0;
}

int handle_LIST(int sockfd)
{
  FILE* pipe_fp = NULL;
  char list_command[MAX_BUF_SIZE];
  char list_data[MAX_BUF_SIZE * 10];

  memset(list_data, 0, sizeof(list_data)); // initialize buffer
  getcwd(current_path, sizeof(current_path));
  snprintf(list_command, MAX_BUF_SIZE, "ls -l %s", current_path);
  if((pipe_fp = popen(list_command, "r")) == NULL)
  {
    send_to_socket(&sockfd, server_response_451);
    return 1;
  }
  fread(list_data, MAX_BUF_SIZE * 10, 1, pipe_fp);
  pclose(pipe_fp);

  send_to_socket(&sockfd, server_response_150);

  int data_client_sock;
  struct sockaddr_in data_client_addr;
  unsigned int data_client_addr_size = sizeof(data_client_addr);
  data_client_sock = accept(data_transfer_sock, (struct sockaddr *) &data_client_addr, &data_client_addr_size);

  send_to_socket(&data_client_sock, list_data);
  send_to_socket(&sockfd, server_response_226);

  close_socket(&data_client_sock);
  close_socket(&data_transfer_sock);

  return 0;
}

int handle_RETR(int* sockfd, bool reverse)
{
  send_to_socket(sockfd, server_response_150); 

  int data_client_sock;
  struct sockaddr_in data_client_addr;
  unsigned int data_client_addr_size = sizeof(data_client_addr);
  data_client_sock = accept(data_transfer_sock, (struct sockaddr *) &data_client_addr, &data_client_addr_size);

  char file_name[MAX_BUF_SIZE];
  get_token(file_name, msg_buf, 1);
  send_file(&data_client_sock, file_name, reverse);

  send_to_socket(sockfd, server_response_226);

  close_socket(&data_client_sock);
  close_socket(&data_transfer_sock);

  return 0;
}

int handle_STOR(int* sockfd)
{
  send_to_socket(sockfd, server_response_150); 

  int data_client_sock;
  struct sockaddr_in data_client_addr;
  unsigned int data_client_addr_size = sizeof(data_client_addr);
  data_client_sock = accept(data_transfer_sock, (struct sockaddr *) &data_client_addr, &data_client_addr_size);

  char file_name[MAX_BUF_SIZE];
  get_token(file_name, msg_buf, 1);

  FILE* file = fopen(file_name, "wb");
  receive_file(&data_client_sock, file);
  fclose(file);

  send_to_socket(sockfd, server_response_226);

  close_socket(&data_client_sock);
  close_socket(&data_transfer_sock);

  return 0;
}

void quit()
{
  close_socket(&client_sock);
  close_socket(&listen_sock);
  exit(0);
}
