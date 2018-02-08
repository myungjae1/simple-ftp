#ifndef __SOCKET__UTIL__
#define __SOCKET__UTIL__

#include "constant.h"

#include <stdio.h>

int close_socket(int*);

int open_data_socket(int*);

int send_to_socket(int*, char*);
int receive_from_socket(int*, char*);

int send_file(int*, char*, bool);
int receive_file(int*, FILE*);

#endif
