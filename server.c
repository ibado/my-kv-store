#include "kv.h"
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_CONNECTIONS 256
#define MSG_MAX_BUFFER_SIZE 256

#define ok_or_return(fd)                                                       \
  do {                                                                         \
    if ((fd) == -1) {                                                          \
      return -1;                                                               \
    }                                                                          \
  } while (0)

#define ok_or_break(fd, msg)                                                   \
  do {                                                                         \
    if ((fd) == -1) {                                                          \
      perror((msg));                                                           \
      break;                                                                   \
    }                                                                          \
  } while (0)

#define ok_or_perror(fd, msg)                                                  \
  do {                                                                         \
    if ((fd) == -1) {                                                          \
      perror((msg));                                                           \
    }                                                                          \
  } while (0)

int start_tcp_sock(int port) {
  int s = socket(AF_INET, SOCK_STREAM, 0);
  ok_or_return(s);
  int yes = 1;
  setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
  struct sockaddr_in sock_addr = {0};
  sock_addr.sin_family = AF_INET;
  sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  sock_addr.sin_port = htons(port);
  ok_or_return(bind(s, (struct sockaddr *)&sock_addr, sizeof(sock_addr)));
  ok_or_return(listen(s, MAX_CONNECTIONS));
  return s;
}

int send_str(int fd, const char *str, size_t len) {
  char msg[len + 1];
  memcpy(msg, str, len);
  msg[len] = '\n';
  return send(fd, msg, len + 1, 0);
}

void sigint_handler() {
  exit(1); // calling exit so atexit registers work
}

static kv_store kv;
static size_t fds_len = 0;
static size_t fds_cap = 10;
static struct pollfd *fds;

void cleanup() {
  for (size_t i = 0; i < fds_len; i++) {
    printf("closing fd%ld\n", i);
    ok_or_perror(close(fds[i].fd), "error closing socket fd");
  }
  free(fds);
  kv_close(&kv);
}

int main() {
  signal(SIGINT, sigint_handler);
  kv = kv_init("server-kv");
  int server_fd = start_tcp_sock(8080);
  atexit(cleanup);

  // TODO: implement a dynamic array for this
  fds = calloc(fds_cap, sizeof(struct pollfd));

  fds[0].fd = server_fd;
  fds[0].events = POLLIN;
  fds_len = 1;

  puts("Accepting request...");
  for (;;) {
    int poll_count = poll(fds, fds_len, -1);
    ok_or_break(poll_count, "poll error");
    // 0 is timeout but can't be since passed -1 to poll (wait for ever)
    assert(poll_count != 0);

    for (size_t i = 0; i < fds_len; i++) {
      if (fds[i].revents & (POLLIN | POLLHUP)) {
        if (fds[i].fd == server_fd) {
          puts("new client connecting");
          struct sockaddr_storage clientaddr = {0};
          socklen_t addrlen = sizeof(clientaddr);
          int cd = accept(server_fd, (struct sockaddr *)&clientaddr, &addrlen);
          ok_or_break(cd, "ups, accept error");
          if (fds_cap == fds_len) {
            // TODO: implement realloc or dynamic array
            send_str(cd, "Sorry, client list is full!", 28);
            ok_or_perror(close(cd), "error closing client fd");
            continue;
          }
          fds[fds_len].fd = cd;
          fds[fds_len].events = POLLIN;
          fds_len++;
          send_str(cd, "Welcom to my-kv-store!", 23);
        } else {
          char buf[MSG_MAX_BUFFER_SIZE] = {0};
          int r = recv(fds[i].fd, buf, MSG_MAX_BUFFER_SIZE, 0);
          ok_or_break(r, "recv error!");
          if (r == 0) {
            ok_or_perror(close(fds[i].fd), "error closing client fd");
            fds[i] = fds[fds_len - 1];
            fds_len--;
            puts("client closed connection");
          } else {
            printf("client says: %s\n", buf);
            int cd = fds[i].fd;
            char *msg = strtok(buf, "\r\n");
            char *arg1 = strtok(msg, " ");
            char *arg2 = strtok(NULL, " ");
            char *arg3 = strtok(NULL, "\0");
            if (strcmp(arg1, "get") == 0 && arg2) {
              buffer out;
              if (kv_get(&kv, arg2, &out)) {
                send_str(cd, out.data, out.len);
              } else {
                send_str(cd, "nil", 4);
              }
            } else if (strcmp(arg1, "put") == 0 && arg2) {
              if (arg3 == NULL) {
                send_str(cd, "bad cmd! value is missing", 26);
                continue;
              }
              kv_put(&kv, arg2, arg3);
            } else if (strcmp(arg1, "del") == 0 && arg2) {
              if (kv_del(&kv, arg2)) {
                send_str(cd, "OK", 3);
              } else {
                send_str(cd, "nil", 4);
              }
            } else if (strcmp(arg1, "dbg") == 0) {
              kv_dbg(&kv);
            } else {
              send_str(cd, "Invalid command!", 17);
            }
          }
        }
      }
    }
  }
}
