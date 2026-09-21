#include "lab.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *get_greeting(const char *restrict name)
{
  if (name == NULL)
  {
    return NULL;
  }

  // Allocate memory for the greeting message
  int length = snprintf(NULL, 0, "Hello, %s!", name);
  if (length < 0) // GCOVR_EXCL_START
  {
    return NULL; // snprintf failed
  } // GCOVR_EXCL_STOP

  //Casting is safe here because we know length is non-negative
  size_t alloc_size = (size_t) length + 1; // +1 for the null terminator
  char *greeting = malloc( alloc_size);


  if (greeting == NULL) // GCOVR_EXCL_START
  {
    return NULL; // Memory allocation failed
  }  // GCOVR_EXCL_STOP


  // Create the greeting message
  snprintf(greeting, alloc_size, "Hello, %s!", name);

  return greeting;
}

int smtp_parse_reply_code(const char *line)
{
  if (line == NULL)
  {
    return -1;
  }

  if (strlen(line) < 4)
  {
    return -1;
  }

  if (line[0] < '0' || line[0] > '9' ||
      line[1] < '0' || line[1] > '9' ||
      line[2] < '0' || line[2] > '9')
  {
    return -1;
  }

  if (line[3] != ' ' && line[3] != '-')
  {
    return -1;
  }

  return ((line[0] - '0') * 100) +
         ((line[1] - '0') * 10)  +
         (line[2] - '0');
}

int smtp_reply_is_final(const char *line)
{
  if (smtp_parse_reply_code(line) == -1)
  {
    return -1;
  }

  if (line[3] == ' ')
  {
    return 1;
  }

  return 0;
}

char *smtp_dot_stuff(const char *message)
{
  if (message == NULL)
  {
    return NULL;
  }

  size_t length = strlen(message);
  size_t extra_dots = 0;
  int start_of_line = 1;

  for (size_t i = 0; i < length; i++)
  {
    if (start_of_line && message[i] == '.')
    {
      extra_dots++;
    }

    start_of_line = message[i] == '\n';
  }

  char *new_message = malloc(length + extra_dots + 1);
  if (new_message == NULL)
  { // GCOVR_EXCL_START
    return NULL;
  } // GCOVR_EXCL_STOP

  size_t new_index = 0;
  start_of_line = 1;

  for (size_t i = 0; i < length; i++)
  {
    if (start_of_line && message[i] == '.')
    {
      new_message[new_index] = '.';
      new_index++;
    }

    new_message[new_index] = message[i];
    new_index++;
    start_of_line = message[i] == '\n';
  }

  new_message[new_index] = '\0';
  return new_message;
}