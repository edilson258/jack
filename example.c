#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define JACK_IMPLEMENTATION
#include "include/jack.h"

char *read_file_to_buf(char *path) {
  FILE *file = fopen("./file.json", "r");

  if (!file) {
    fprintf(stderr, "Couldn't open %s %s\n", path, strerror(errno));
    exit(1);
  }

  fseek(file, 0, SEEK_END);
  unsigned long file_len = ftell(file);
  fseek(file, 0, SEEK_SET);

  char *content = malloc(sizeof(char) * file_len + 1);
  fread(content, sizeof(char), file_len, file);
  content[file_len] = 0;
  fclose(file);

  return content;
}

int main() {
  char *path = "file.json";
  char *content = read_file_to_buf(path);
  Json json = Json_Parse(content);

  JsonKeyValuePair pair = {
      .key = "hello",
      .value.type = JSON_NUMBER,
      .value.data.number = 69,
  };

  Json_Append(&json, pair);

  char *xs = Json_Stringfy(json, 4);
  printf("%s", xs);

  return 0;
}
