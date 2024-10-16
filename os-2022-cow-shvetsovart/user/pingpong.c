#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"

int main(int argc, char* argv[]) {
  char ping[5] = "ping";
  char pong[5] = "pong";

  int to_parent[2];
  int to_child[2];

  if (pipe(to_parent) == -1) {
    printf("An error occurred while opening the parent pipe\n");
    exit(1);
  }
  if (pipe(to_child) == -1) {
    printf("An error occurred while opening the child pipe\n");
    exit(1);
  }

  int pid = fork();

  if (pid == -1) {
    printf("An error occurred while forking\n");
    exit(1);
  }

  if (pid == 0) {
    // child
    char received[5];
    received[4] = 0;

    int res_read = read(to_child[0], &received, 4);
    while (res_read != 4) {
      res_read += read(to_child[0], &received, 4);
    }

    printf("%d: got %s\n", getpid(), received);

    int res_write = write(to_parent[1], &pong[0], 4);
    while (res_write != 4) {
      res_write += write(to_parent[1], &pong[0], 4);
    }
  } else {
    // parent
    int res_write = write(to_child[1], &ping[0], 4);
    while (res_write != 4) {
      res_write += write(to_child[1], &ping[0], 4);
    }

    char received[5];
    received[4] = 0;

    int res_read = read(to_parent[0], &received, 4);
    while (res_read != 4) {
      res_read = +read(to_parent[0], &received, 4);
    }

    printf("%d: got %s\n", getpid(), received);
  }

  close(to_child[0]);
  close(to_child[1]);
  close(to_parent[0]);
  close(to_parent[1]);

  exit(0);
}
