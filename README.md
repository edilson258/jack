# Jack

Jack is header-only library for parsing JSON data in C.

### Examples

```C
#define JACK_IMPLEMENTATION
#include "include/jack.h"

int main() {
  char *str = "{\"message\":\"Hello, world!\"}";
  Json json = Json_Parse(str);
  Json_Print(&json, 4);
  return 0;
}

```
- output

```json
{
    "message": "Hello, world!"
}
```

## Brief API explanation

```C
/*
    Will create a new JSON object
*/
Json Json_New();

/*
    Will parse a raw C string to JSON object
    similar to JavaScript's JSON.parse()
*/
Json Json_Parse(char *str);

/*
    Will retrive a key-value pair from JSON object "Json *j"
    with key equal to "char *key", returns NULL if not found.
*/
JsonKeyValuePair *Json_Get(Json *j, char *key);

/*
    Will append a new key-value pair to the JSON object
*/
void Json_Append(Json *j, JsonKeyValuePair pair);

/*
    Will convert a JSON object back to C raw string
    similar to JavaScript's JSON.stringfy()
*/
char *Json_Stringfy(Json obj, unsigned int depth);

/*
    Will print the JSON object to the stdout
*/
void Json_Print(Json *j, int depth);
```
- ***Note***: more details about each API function may be found in doc strings

## Features

Jack has support for parsing JSON:

- [x] Number
- [x] String
- [x] Arrays
- [x] Nested & complex JSON objects
- [x] Number starting with `-` or `+`
- [x] Boolean values 
- [x] Null value 

## Try it now

1. Download the `include/jack.h` header file

```shell
wget https://raw.githubusercontent.com/edilson258/jack/master/include/jack.h
```

2. Include in your program

```C
#define JACK_IMPLEMENTATION
#include "jack.h"

int main() {
  // your trash code goes here
}
```
- ***Note***: The examples above expects the `jack.h` header file to be in the same directory as your program.


## Contribuitions

Feel free to play with it or maybe send me some PRs!
