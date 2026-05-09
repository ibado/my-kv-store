#include "kv.h"
#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <bits/types/struct_timeval.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_CONNECTIONS 256
#define MSG_MAX_BUFFER_SIZE 256

#define check_err(fd, msg)                                                     \
  do {                                                                         \
    if ((fd) == -1) {                                                          \
      perror((msg));                                                           \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

static volatile bool running = true;

void int_handler() {
  puts("bye bye!");
  running = false;
}

int start_tcp_sock(int port) {
  int s = socket(AF_INET, SOCK_STREAM, 0);
  check_err(s, "error creating socket");
  int yes = 1;
  setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
  struct sockaddr_in sock_addr = {0};
  sock_addr.sin_family = AF_INET;
  sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  sock_addr.sin_port = htons(port);
  int bind_res = bind(s, (struct sockaddr *)&sock_addr, sizeof(sock_addr));
  check_err(bind_res, "error binding");
  int listen_res = listen(s, MAX_CONNECTIONS);
  check_err(listen_res, "error listening");
  return s;
}

int send_str(int fd, const char *str, size_t len) {
  char msg[len + 1];
  memcpy(msg, str, len);
  msg[len] = '\n';
  return send(fd, msg, len + 1, 0);
}

int main() {
  int s = start_tcp_sock(8080);
  struct sockaddr_storage clientaddr = {0};
  socklen_t addrlen = sizeof(clientaddr);
  puts("Accepting request...");
  int cd = accept(s, (struct sockaddr *)&clientaddr, &addrlen);
  check_err(cd, "ups, accept error");
  send_str(cd, "Hello client!", sizeof("Hello client!"));

  kv_store kv = kv_init("server-kv");
  signal(SIGINT, int_handler);
  while (1) {
    char buf[MSG_MAX_BUFFER_SIZE] = {0};
    recv(cd, buf, MSG_MAX_BUFFER_SIZE, 0);
    if (!running)
      break;
    char *msg = strtok(buf, "\r\n");

    char *arg1 = strtok(msg, " ");
    char *arg2 = strtok(NULL, " ");
    char *arg3 = strtok(NULL, "\0");
    assert(arg1);
    if (strcmp(arg1, "get") == 0 && arg2) {
      buffer out;
      if (kv_get(&kv, arg2, &out)) {
        send_str(cd, out.data, out.len);
      } else {
        send_str(cd, "nil", sizeof("nil"));
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

  kv_close(&kv);
  check_err(shutdown(cd, SHUT_RDWR), "error shutting down client sock");
  check_err(shutdown(s, SHUT_RDWR), "error shutting down server sock");
}
