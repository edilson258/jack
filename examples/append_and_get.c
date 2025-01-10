#include <stdio.h>

#define JACK_IMPLEMENTATION
#include <jack.h>

int main(void)
{
  jjson_t json;
  jjson_init(&json);

  JsonKeyValuePair kv = {.key = "hello", .value.type = JSON_STRING, .value.data.string = "Hello, world!"};
  Json_Append(&json, kv);

  char *message;
  enum jack_error err = Json_Get_String(&json, "hello", &message);
  if (JACK_OK == err)
  {
    printf("%s\n", message);
  }
}
