#ifndef __JACK_JSON_PARSER__
#define __JACK_JSON_PARSER__

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum
{
  JSON_FALSE = 0,
  JSON_TRUE = 1,
} jjson_bool_t;

typedef enum
{
  JSON_STRING = 1,
  JSON_NUMBER = 2,
  JSON_ARRAY = 3,
  JSON_OBJECT = 4,
  JSON_NULL = 5,
  JSON_BOOLEAN = 6,
} jjson_type_t;

typedef struct
{
  size_t length;
  size_t capacity;
  struct jjson_val_t *items;
} jjson_array_t;

typedef struct jjson_val_t
{
  jjson_type_t type;
  union
  {
    long long number;
    char *string;
    jjson_array_t array;
    struct jjson_t *object;
    jjson_bool_t boolean;
  } data;
} jjson_val_t;

typedef struct
{
  char *key;
  jjson_val_t value;
} jjson_kv_t;

typedef struct jjson_t
{
  size_t capacity;
  size_t field_count;
  jjson_kv_t *fields;
} jjson_t;

enum jjson_error
{
  JJE_OK = 0,
  JJE_ALLOC_FAIL = -1,
  JJE_NOT_FOUND = -2,
};

enum jjson_error jjson_init(jjson_t *json);
enum jjson_error jjson_parse(jjson_t *json, char *raw);

enum jjson_error jjson_get(jjson_t *json, char *key, jjson_val_t **out);
enum jjson_error jjson_get_string(jjson_t *json, char *key, char **out);
enum jjson_error jjson_get_number(jjson_t *json, char *key, long long **out);

enum jjson_error jjson_add(jjson_t *json, jjson_kv_t kv);
enum jjson_error jjson_add_string(jjson_t *json, char *key, char *value);
enum jjson_error jjson_add_number(jjson_t *json, char *key, long long value);

char *jjson_stringify(jjson_t *obj, unsigned int depth);
void jjson_dump(jjson_t *json, int depth);

#define JACK_IMPLEMENTATION
#ifdef JACK_IMPLEMENTATION

#define JSON_CAPACITY_INCR_RATE 256

typedef struct
{
  unsigned long line;
  unsigned long colm;
} token_pos_t;

typedef enum
{
  TOKEN_EOF = 1,
  TOKEN_INVALID = 2,

  TOKEN_STRING = 3,
  TOKEN_NUMBER = 4,
  TOKEN_NULL = 5,
  TOKEN_TRUE = 6,
  TOKEN_FALSE = 7,

  TOKEN_COLON = ':',
  TOKEN_COMMA = ',',
  TOKEN_LBRACE = '{',
  TOKEN_RBRACE = '}',
  TOKEN_LPAREN = '[',
  TOKEN_RPAREN = ']',
} TokenType;

typedef struct
{
  TokenType type;
  union
  {
    char *string;
    char chr;
    long number;
    unsigned int boolean;
  } label;
  token_pos_t pos;
} jjson_token_t;

#define TOKEN_TYPE(tt)                                                                                                 \
  ((tt) == TOKEN_EOF       ? "EOF"                                                                                     \
   : (tt) == TOKEN_INVALID ? "Invalid JSON token"                                                                      \
   : (tt) == TOKEN_STRING  ? "String"                                                                                  \
   : (tt) == TOKEN_NUMBER  ? "Number"                                                                                  \
   : (tt) == TOKEN_COMMA   ? ","                                                                                       \
   : (tt) == TOKEN_COLON   ? ":"                                                                                       \
   : (tt) == TOKEN_LBRACE  ? "{"                                                                                       \
   : (tt) == TOKEN_RBRACE  ? "}"                                                                                       \
   : (tt) == TOKEN_LPAREN  ? "["                                                                                       \
   : (tt) == TOKEN_RPAREN  ? "]"                                                                                       \
                           : "Unkown JSON type")

typedef struct
{
  size_t line;
  size_t colm;

  char *content;
  char curr_char;
  size_t content_len;

  size_t pos;
  size_t read_pos;
} jjson_lexer_t;

typedef struct
{
  jjson_lexer_t lexer;
  jjson_token_t curr_token;
  jjson_token_t next_token;
} jjson_parser_t;

typedef struct
{
  FILE *stream;
  size_t tab;
  size_t tab_rate;
} jjson_stringfier_t;

jjson_lexer_t lexer_new(char *content);
jjson_token_t lexer_next_token(jjson_lexer_t *l);

enum jjson_error jjson_init(jjson_t *json)
{
  json->field_count = 0;
  json->capacity = JSON_CAPACITY_INCR_RATE;
  json->fields = (jjson_kv_t *)malloc(sizeof(jjson_kv_t) * JSON_CAPACITY_INCR_RATE);
  return JJE_OK;
}

enum jjson_error jjson_get(jjson_t *json, char *key, jjson_val_t **out)
{
  for (size_t i = 0; i < json->field_count; ++i)
  {
    jjson_kv_t *tmp = &json->fields[i];
    if (strcmp(key, tmp->key) == 0)
    {
      *out = &tmp->value;
      return JJE_OK;
    }
  }
  return JJE_NOT_FOUND;
}

enum jjson_error jjson_get_string(jjson_t *json, char *key, char **out)
{
  jjson_val_t *value = NULL;

  enum jjson_error error = jjson_get(json, key, &value);
  if (JJE_OK != error || JSON_STRING != value->type)
  {
    return error;
  }

  *out = value->data.string;
  return JJE_OK;
}

enum jjson_error jjson_get_number(jjson_t *json, char *key, long long **out)
{
  jjson_val_t *value = NULL;

  enum jjson_error error = jjson_get(json, key, &value);
  if (JJE_OK != error || JSON_NUMBER != value->type)
  {
    return error;
  }

  *out = &value->data.number;
  return JJE_OK;
}

enum jjson_error jjson_add(jjson_t *json, jjson_kv_t kv)
{
  if (json->capacity <= json->field_count)
  {
    size_t new_cap = json->capacity + JSON_CAPACITY_INCR_RATE;
    json->fields = (jjson_kv_t *)realloc(json->fields, sizeof(jjson_kv_t) * new_cap);
    json->capacity = new_cap;
  }
  json->fields[json->field_count++] = kv;
  return JJE_OK;
}

enum jjson_error jjson_add_string(jjson_t *json, char *key, char *value)
{
  jjson_kv_t kv = {.key = key, .value.type = JSON_STRING, .value.data.string = value};
  return jjson_add(json, kv);
}

enum jjson_error jjson_add_number(jjson_t *json, char *key, long long value)
{
  jjson_kv_t kv = {.key = key, .value.type = JSON_NUMBER, .value.data.number = value};
  return jjson_add(json, kv);
}

#define LEXER_EOF '\0'
typedef int (*lexer_read_predicate)(int);

void lexer_read_char(jjson_lexer_t *l);
void lexer_drop_while(jjson_lexer_t *l, lexer_read_predicate pred);
char *lexer_read_upto(jjson_lexer_t *l, char stop_char);
void lexer_skip_whitespace(jjson_lexer_t *l);

jjson_lexer_t lexer_new(char *content)
{
  jjson_lexer_t l = {0};
  l.content = content;
  l.content_len = strlen(content);
  l.line = 1;
  lexer_read_char(&l);
  return l;
}

void lexer_read_char(jjson_lexer_t *l)
{
  if (l->read_pos >= l->content_len)
  {
    l->curr_char = LEXER_EOF;
    l->pos = l->read_pos;
    return;
  }

  l->curr_char = l->content[l->read_pos];
  l->pos = l->read_pos;
  l->read_pos += 1;

  if (l->curr_char == '\n')
  {
    l->line += 1;
    l->colm = 0;
  }
  else
  {
    l->colm += 1;
  }
}

void lexer_drop_while(jjson_lexer_t *l, lexer_read_predicate pred)
{
  while (l->curr_char != LEXER_EOF && pred(l->curr_char))
  {
    lexer_read_char(l);
  }
}

char *lexer_read_while(jjson_lexer_t *l, lexer_read_predicate pred)
{
  size_t start = l->pos;
  while (l->curr_char != LEXER_EOF && pred(l->curr_char))
  {
    lexer_read_char(l);
  }
  size_t buf_len = l->pos - start;
  char *buf = (char *)malloc(sizeof(char) * (buf_len + 1));
  strncpy(buf, l->content + start, buf_len);
  buf[buf_len] = '\0';
  return buf;
}

void lexer_skip_whitespace(jjson_lexer_t *l) { lexer_drop_while(l, isspace); }

int is_not_unquote(int c) { return c != '"'; }

jjson_token_t lexer_next_token(jjson_lexer_t *l)
{
  lexer_skip_whitespace(l);

  jjson_token_t token;
  token.pos.line = l->line;
  token.pos.colm = l->colm;

  if (l->curr_char == LEXER_EOF)
  {
    token.type = TOKEN_EOF;
    return token;
  }

  switch (l->curr_char)
  {
  case '{':
    lexer_read_char(l);
    token.type = TOKEN_LBRACE;
    return token;
  case '}':
    lexer_read_char(l);
    token.type = TOKEN_RBRACE;
    return token;
  case '[':
    lexer_read_char(l);
    token.type = TOKEN_LPAREN;
    return token;
  case ']':
    lexer_read_char(l);
    token.type = TOKEN_RPAREN;
    return token;
  case ':':
    lexer_read_char(l);
    token.type = TOKEN_COLON;
    return token;
  case ',':
    lexer_read_char(l);
    token.type = TOKEN_COMMA;
    return token;
  case '"':
    lexer_read_char(l);
    token.type = TOKEN_STRING;
    token.label.string = lexer_read_while(l, is_not_unquote);
    lexer_read_char(l);
    return token;
  }

  if (isalpha(l->curr_char))
  {
    char *buf = lexer_read_while(l, isalpha);

    if (strcmp(buf, "null") == 0)
    {
      token.type = TOKEN_NULL;
      return token;
    }

    if (strcmp(buf, "true") == 0)
    {
      token.type = TOKEN_TRUE;
      return token;
    }

    if (strcmp(buf, "false") == 0)
    {
      token.type = TOKEN_FALSE;
      return token;
    }

    token.type = TOKEN_INVALID;
    token.label.chr = buf[0];
    return token;
  }

  // parsing `+69` as `69`
  if (l->curr_char == '+' && isdigit(l->content[l->read_pos]))
  {
    lexer_read_char(l);
  }

  if (isdigit(l->curr_char) || ((l->curr_char == '-') && isdigit(l->content[l->read_pos])))
  {

    int has_prefix = 0;
    if (l->curr_char == '-')
    {
      lexer_read_char(l);
      has_prefix = 1;
    }

    char *buf = lexer_read_while(l, isdigit);
    token.type = TOKEN_NUMBER;
    token.label.number = strtol(buf, NULL, 0);

    if (has_prefix)
    {
      token.label.number -= (token.label.number * 2);
    }

    free(buf);
    return token;
  }

  token.type = TOKEN_INVALID;
  token.label.chr = l->curr_char;
  lexer_read_char(l);
  return token;
}

void parser_bump(jjson_parser_t *p);
void parser_bump_expected(jjson_parser_t *p, TokenType tt);

// Parsers
enum jjson_error parse_json_object(jjson_parser_t *p, jjson_t *json);
jjson_val_t parse_json_value(jjson_parser_t *p);
jjson_array_t parse_json_array(jjson_parser_t *p);
jjson_kv_t parse_json_key_value(jjson_parser_t *p);

enum jjson_error jjson_parse(jjson_t *json, char *raw)
{
  jjson_parser_t p = {.lexer = lexer_new(raw)};
  parser_bump(&p);
  parser_bump(&p);
  return parse_json_object(&p, json);
}

enum jjson_error parse_json_object(jjson_parser_t *p, jjson_t *json)
{
  if (p->curr_token.type == TOKEN_EOF)
  {
    return JJE_OK;
  }
  parser_bump_expected(p, TOKEN_LBRACE);
  if (p->curr_token.type == TOKEN_RBRACE)
  {
    return JJE_OK;
  }
  while (1)
  {
    jjson_kv_t pair = parse_json_key_value(p);
    jjson_add(json, pair);
    if (p->curr_token.type == TOKEN_COMMA)
    {
      parser_bump(p);
    }
    if (p->curr_token.type == TOKEN_RBRACE)
    {
      break;
    }
    if (p->curr_token.type == TOKEN_EOF)
    {
      break;
    }
  }
  return JJE_OK;
}

jjson_kv_t parse_json_key_value(jjson_parser_t *p)
{
  jjson_kv_t pair;
  if (p->curr_token.type != TOKEN_STRING)
  {
    printf("%s\n", TOKEN_TYPE(p->curr_token.type));
    fprintf(stderr, "[JSON ERROR]: Expected JSON key to be string at %lu:%lu\n", p->curr_token.pos.line,
            p->curr_token.pos.colm);
    exit(1);
  }
  pair.key = p->curr_token.label.string;
  parser_bump(p);
  parser_bump_expected(p, TOKEN_COLON);
  pair.value = parse_json_value(p);
  return pair;
}

jjson_val_t parse_json_value(jjson_parser_t *p)
{
  jjson_val_t value;
  switch (p->curr_token.type)
  {
  case TOKEN_NUMBER:
    value.type = JSON_NUMBER;
    value.data.number = p->curr_token.label.number;
    break;
  case TOKEN_STRING:
    value.type = JSON_STRING;
    value.data.string = p->curr_token.label.string;
    break;
  case TOKEN_LPAREN:
    value.type = JSON_ARRAY;
    value.data.array = parse_json_array(p);
    break;
  case TOKEN_LBRACE:
    value.type = JSON_OBJECT;
    value.data.object = (jjson_t *)malloc(sizeof(jjson_t));
    jjson_init(value.data.object);
    parse_json_object(p, value.data.object);
    break;
  case TOKEN_NULL:
    value.type = JSON_NULL;
    break;
  case TOKEN_TRUE:
    value.type = JSON_BOOLEAN;
    value.data.boolean = JSON_TRUE;
    break;
  case TOKEN_FALSE:
    value.type = JSON_BOOLEAN;
    value.data.boolean = JSON_FALSE;
    break;
  default:
    fprintf(stderr, "[JSON ERROR]: Unsupported JSON value at %lu:%lu\n", p->curr_token.pos.line,
            p->curr_token.pos.colm);
    exit(1);
  }
  parser_bump(p);
  return value;
}

jjson_array_t JsonArray_New()
{
  jjson_array_t array;
  array.length = 0;
  array.capacity = JSON_CAPACITY_INCR_RATE;
  array.items = (jjson_val_t *)malloc(sizeof(jjson_t) * JSON_CAPACITY_INCR_RATE);
  return array;
}

void JsonArray_Append(jjson_array_t *array, jjson_val_t val)
{
  if (array->capacity >= array->length)
  {
    size_t new_cap = array->capacity + JSON_CAPACITY_INCR_RATE;
    array->items = (jjson_val_t *)realloc(array->items, sizeof(jjson_val_t) * new_cap);
    array->capacity = new_cap;
  }
  array->items[array->length++] = val;
}

jjson_array_t parse_json_array(jjson_parser_t *p)
{
  parser_bump_expected(p, TOKEN_LPAREN);
  jjson_array_t array = JsonArray_New();
  while (p->curr_token.type != TOKEN_EOF && p->curr_token.type != TOKEN_RPAREN)
  {
    JsonArray_Append(&array, parse_json_value(p));
    if (p->curr_token.type == TOKEN_RPAREN)
    {
      break;
    }
    if (p->curr_token.type == TOKEN_COMMA)
    {
      parser_bump(p);
    }
    else
    {
      fprintf(stderr,
              "[JSON ERROR]: Expected ',' to separated Json Array items at "
              "%lu:%lu\n",
              p->curr_token.pos.line, p->curr_token.pos.colm);
      exit(1);
    }
  }

  return array;
}

void parser_bump(jjson_parser_t *p)
{
  p->curr_token = p->next_token;
  jjson_token_t tkn = lexer_next_token(&p->lexer);
  switch (tkn.type)
  {
  case TOKEN_INVALID:
    fprintf(stderr, "[JSON ERROR]: Invalid symbol '%c' at %lu:%lu\n", tkn.label.chr, tkn.pos.line, tkn.pos.colm);
    exit(1);
  default:
    p->next_token = tkn;
    break;
  }
}

void parser_bump_expected(jjson_parser_t *p, TokenType tt)
{
  if (p->curr_token.type != tt)
  {
    fprintf(stderr, "[JSON ERROR]: Expected '%s' but got '%s' at %lu:%lu\n", TOKEN_TYPE(tt),
            TOKEN_TYPE(p->curr_token.type), p->curr_token.pos.line, p->curr_token.pos.colm);
    exit(1);
  }
  parser_bump(p);
}

void stringify_json_object(jjson_stringfier_t *ctx, jjson_t *obj);
void stringify_json_value(jjson_stringfier_t *ctx, jjson_val_t val);
void stringify_json_array(jjson_stringfier_t *ctx, jjson_array_t arr);
void stringfier_print_tab(jjson_stringfier_t *ctx);

char *jjson_stringify(jjson_t *obj, unsigned int depth)
{
  char *buf = NULL;
  unsigned long buf_len = 0;
  jjson_stringfier_t ctx;
  ctx.tab_rate = depth;
  ctx.tab = depth;
  ctx.stream = open_memstream(&buf, &buf_len);
  stringify_json_object(&ctx, obj);
  fclose(ctx.stream);
  return buf;
}

void stringify_json_object(jjson_stringfier_t *ctx, jjson_t *obj)
{
  fprintf(ctx->stream, "{\n");
  for (unsigned long i = 0; i < obj->field_count; ++i)
  {
    stringfier_print_tab(ctx);
    fprintf(ctx->stream, "\"%s\": ", obj->fields[i].key);
    stringify_json_value(ctx, obj->fields[i].value);
    if (i + 1 < obj->field_count)
    {
      fprintf(ctx->stream, ",");
    }
    fprintf(ctx->stream, "\n");
  }
  ctx->tab == ctx->tab_rate ? fprintf(ctx->stream, "}\n") : ({
    ctx->tab -= ctx->tab_rate;
    stringfier_print_tab(ctx);
    fprintf(ctx->stream, "}");
    ctx->tab += ctx->tab_rate;
  });
}

void stringify_json_value(jjson_stringfier_t *ctx, jjson_val_t val)
{
  switch (val.type)
  {
  case JSON_NUMBER:
    fprintf(ctx->stream, "%lld", val.data.number);
    return;
  case JSON_STRING:
    fprintf(ctx->stream, "\"%s\"", val.data.string);
    return;
  case JSON_ARRAY:
    stringify_json_array(ctx, val.data.array);
    return;
  case JSON_OBJECT:
    ctx->tab += ctx->tab_rate;
    stringify_json_object(ctx, val.data.object);
    ctx->tab -= ctx->tab_rate;
    return;
  case JSON_NULL:
    fprintf(ctx->stream, "null");
    return;
  case JSON_BOOLEAN:
    if (val.data.boolean)
    {
      fprintf(ctx->stream, "true");
    }
    else
    {
      fprintf(ctx->stream, "false");
    }
    return;
  }
}

void stringify_json_array(jjson_stringfier_t *ctx, jjson_array_t arr)
{
  fprintf(ctx->stream, "[");
  ctx->tab += ctx->tab_rate;
  for (int i = 0; i < arr.length; ++i)
  {
    fprintf(ctx->stream, "\n");
    stringfier_print_tab(ctx);
    stringify_json_value(ctx, arr.items[i]);
    if (i + 1 < arr.length)
    {
      fprintf(ctx->stream, ",");
    }
  }
  ctx->tab -= ctx->tab_rate;
  fprintf(ctx->stream, "\n");
  stringfier_print_tab(ctx);
  fprintf(ctx->stream, "]");
}

void stringfier_print_tab(jjson_stringfier_t *ctx)
{
  for (unsigned long i = 0; i < ctx->tab; ++i)
  {
    fprintf(ctx->stream, " ");
  }
}

void jjson_dump(jjson_t *json, int depth)
{
  char *buf = jjson_stringify(json, depth);
  printf("%s", buf);
  free(buf);
}
#endif // JACK_IMPLEMENTATION

#endif // __JACK_JSON_PARSER__
