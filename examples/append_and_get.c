#include <stdio.h>

#define JACK_IMPLEMENTATION
#include <jack.h>

int main(void)
{
  Json json = Json_New();

  JsonKeyValuePair kv = {.key = "hello", .value.type = JSON_STRING, .value.data.string = "Hello, world!"};
  Json_Append(&json, kv);

  char *message;
  enum jack_error err = Json_Get_String(&json, "hello", &message);
  if (JACK_OK == err)
  {
    printf("%s\n", message);
  }
}
