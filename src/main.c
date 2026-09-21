#include "lab.h"

#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#ifdef TEST
#define main main_exclude
#endif

static void print_usage(FILE *stream)
{
  fprintf(stream,
          "Usage: myapp -f <from> -t <to> [-s subject] [-b body] [-p port]\n"
          "          [-H helo-host] <server>\n");
}

static int contains_newline(const char *text)
{
  if (text == NULL)
  {
    return 0;
  }

  return strchr(text, '\r') != NULL || strchr(text, '\n') != NULL;
}

static char *read_body(void)
{
  size_t capacity = 1024;
  size_t length = 0;
  char *body = malloc(capacity);

  if (body == NULL)
  {
    return NULL;
  }

  int character;

  while ((character = fgetc(stdin)) != EOF)
  {
    if (length + 1 >= capacity)
    {
      size_t new_capacity = capacity * 2;
      char *larger_body = realloc(body, new_capacity);

      if (larger_body == NULL)
      {
        free(body);
        return NULL;
      }

      body = larger_body;
      capacity = new_capacity;
    }

    body[length] = (char)character;
    length++;
  }

  if (ferror(stdin))
  {
    free(body);
    return NULL;
  }

  body[length] = '\0';
  return body;
}

static int connect_to_server(const char *server, const char *port)
{
  struct addrinfo hints;
  struct addrinfo *addresses = NULL;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  int lookup_result = getaddrinfo(server, port, &hints, &addresses);

  if (lookup_result != 0)
  {
    fprintf(stderr, "Could not find server: %s\n",
            gai_strerror(lookup_result));
    return -1;
  }

  int socket_fd = -1;

  for (struct addrinfo *address = addresses;
       address != NULL;
       address = address->ai_next)
  {
    socket_fd = socket(address->ai_family,
                       address->ai_socktype,
                       address->ai_protocol);

    if (socket_fd < 0)
    {
      continue;
    }

    if (connect(socket_fd, address->ai_addr, address->ai_addrlen) == 0)
    {
      break;
    }

    close(socket_fd);
    socket_fd = -1;
  }

  freeaddrinfo(addresses);
  return socket_fd;
}

static ssize_t socket_read(void *context, void *buffer, size_t length)
{
  int socket_fd = *(int *)context;
  return recv(socket_fd, buffer, length, 0);
}

static ssize_t socket_write(void *context,
                            const void *buffer,
                            size_t length)
{
  int socket_fd = *(int *)context;
  return send(socket_fd, buffer, length, 0);
}

int main(int argc, char **argv)
{
  const char *from = NULL;
  const char *to = NULL;
  const char *subject = "";
  const char *body = NULL;
  const char *port = "25";
  const char *helo_host = "localhost";

  if (argc == 1)
  {
    print_usage(stdout);
    return 0;
  }

  opterr = 0;
  int option;

  while ((option = getopt(argc, argv, "f:t:s:b:p:H:")) != -1)
  {
    switch (option)
    {
      case 'f':
        from = optarg;
        break;

      case 't':
        to = optarg;
        break;

      case 's':
        subject = optarg;
        break;

      case 'b':
        body = optarg;
        break;

      case 'p':
        port = optarg;
        break;

      case 'H':
        helo_host = optarg;
        break;

      default:
        print_usage(stderr);
        return 1;
    }
  }

  if (from == NULL || to == NULL || optind != argc - 1)
  {
    print_usage(stderr);
    return 1;
  }

  if (contains_newline(from) ||
      contains_newline(to) ||
      contains_newline(subject) ||
      contains_newline(helo_host))
  {
    fprintf(stderr,
            "Email addresses, subject, and HELO host cannot contain newlines.\n");
    return 1;
  }

  char *stdin_body = NULL;

  if (body == NULL)
  {
    stdin_body = read_body();

    if (stdin_body == NULL)
    {
      fprintf(stderr, "Could not read the email body.\n");
      return 2;
    }

    body = stdin_body;
  }

  const char *server = argv[optind];
  int socket_fd = connect_to_server(server, port);

  if (socket_fd < 0)
  {
    fprintf(stderr, "Could not connect to %s on port %s.\n",
            server, port);
    free(stdin_body);
    return 2;
  }

  int session_result = smtp_run_session(
      socket_read,
      socket_write,
      &socket_fd,
      helo_host,
      from,
      to,
      subject,
      body);

  close(socket_fd);
  free(stdin_body);

  if (session_result != 0)
  {
    fprintf(stderr, "The SMTP session failed.\n");
    return 2;
  }

  return 0;
}