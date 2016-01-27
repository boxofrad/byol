#include <stdio.h>

static char input[2048];

int main(int argc, char **argv) {
  puts("BYOL Version 0.0.1");
  puts("Press CTRL-C to Exit\n");

  while (1) {
    fputs("byol> ", stdout);
    fgets(input, 2048, stdin);
    printf("No you're a %s", input);
  }

  return 0;
}
