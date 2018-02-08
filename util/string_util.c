#include "constant.h"

#include <ctype.h>

void get_token(char* token, char* str, int index)
{
  char* p = token;
  char* p2 = str;

  while(index > 0) 
  {
    // skip precedent token
    while(*p2 && *p2 != ' ' && *p2 != '\r' && *p2 != '\n')
    {
      p2++;
    }

    // skip a blank
    p2++;

    index--;
  }

  // copy the token
  while (*p2 && *p2 != ' ' && *p2 != '\r' && *p2 != '\n') {
    *p++ = *p2++;
  }

  // add null character to specify the end of string
  *p = '\0'; 
}

char* parse_number(char* p, int* number)
{
  *number = 0;

  while(!isdigit(*p))
  {
    p++;
  }

  while(isdigit(*p))
  {
    *number = 10*(*number) + (*p - '0');
    p++;
  }

  return p;
}

void parse_server_data_channel_info(char* server_response, unsigned int* server_data_ip, unsigned int* server_data_port)
{
  char* p = server_response;
  int i;
  int number;

  *server_data_ip = 0;
  *server_data_port = 0;

  // skip the first number (response code)
  p = parse_number(p, &number);

  // parse data channel ip address
  for(i = 0 ; i < 4 ; i++)
  {
    p = parse_number(p, &number);
    *server_data_ip = (*server_data_ip << 8) + number;
  }
  
  // parse data channel port
  for(i = 0 ; i < 2 ; i++)
  {
    p = parse_number(p, &number);
    *server_data_port = (*server_data_port << 8) + number;
  }
}
