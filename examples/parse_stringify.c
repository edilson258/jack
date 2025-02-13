#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define JACK_IMPLEMENTATION
#include "../jack.h"

char *read_file_to_buf(char *path)
{
  FILE *file = fopen(path, "r");

  if (!file)
  {
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

int main()
{
  char *path = "test-file.json";
  char *content = read_file_to_buf(path);

  jjson_t json;
  jjson_init(&json);

  enum jjson_error err = jjson_parse(&json, content, strlen(content));
  if (err != JJE_OK)
  {
    printf("Error on parse json, %s\n", jjson_strerror());
    return -1;
  }
  jjson_dump(&json, stdout, 1);

  free(content);
  jjson_deinit(&json);
}
