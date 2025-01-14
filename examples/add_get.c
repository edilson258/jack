#include <stdio.h>

#define JACK_IMPLEMENTATION
#include <jack.h>

int main(void)
{
  jjson_t json;
  jjson_init(&json);

  jjson_add_string(&json, "hello", "Hello from jack!");
  jjson_add_number(&json, "age", 23);

  JJSON_GET_STRING(json, "hello", message);
  if (message)
    printf("%s\n", message);

  JJSON_GET_NUMBER(json, "age", age);
  if (age)
    printf("%lld\n", *age);

  jjson_deinit(&json);
}
