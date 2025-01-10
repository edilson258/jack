#ifndef __JACK_JSON_PARSER__
#define __JACK_JSON_PARSER__

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define JJSON_GET_NUMBER(json, key, varname)                                                                           \
  signed long long *varname;                                                                                           \
  jjson_get_number(&json, key, &varname)

#define JJSON_GET_STRING(json, key, varname)                                                                           \
  char *varname;                                                                                                       \
  jjson_get_string(&json, key, &varname)

typedef enum
{
  JSON_FALSE = 0,
  JSON_TRUE = 1,
} jjson_bool;

typedef enum
{
  JSON_STRING = 1,
  JSON_NUMBER = 2,
  JSON_ARRAY = 3,
  JSON_OBJECT = 4,
  JSON_NULL = 5,
  JSON_BOOLEAN = 6,
} jjson_type;

typedef struct
{
  size_t length;
  size_t capacity;
  struct jjson_value *items;
} jjson_array;

typedef struct jjson_value
{
  jjson_type type;
  union
  {
    long long number;
    char *string;
    jjson_array array;
    struct jjson_t *object;
    jjson_bool boolean;
  } data;
} jjson_value;

typedef struct
{
  char *key;
  jjson_value value;
} jjson_key_value;

typedef struct jjson_t
{
  size_t capacity;
  size_t field_count;
  jjson_key_value *fields;
} jjson_t;

enum jjson_error
{
  JJE_OK = 0,
  JJE_ALLOC_FAIL = -1,
  JJE_NOT_FOUND = -2,
};

enum jjson_error jjson_init(jjson_t *json);
enum jjson_error jjson_parse(jjson_t *json, char *raw);

enum jjson_error jjson_get(jjson_t *json, char *key, jjson_value **out);
enum jjson_error jjson_get_string(jjson_t *json, char *key, char **out);
enum jjson_error jjson_get_number(jjson_t *json, char *key, long long **out);

enum jjson_error jjson_add(jjson_t *json, jjson_key_value kv);
enum jjson_error jjson_add_string(jjson_t *json, char *key, char *value);
enum jjson_error jjson_add_number(jjson_t *json, char *key, long long value);

enum jjson_error jjson_stringify(jjson_t *obj, short depth, char **out);
void jjson_dump(jjson_t *json, FILE *f, int depth);

#define JACK_IMPLEMENTATION
#ifdef JACK_IMPLEMENTATION

#define JSON_CAPACITY_INCR_RATE 256

typedef struct
{
  unsigned long line;
  unsigned long colm;
} jjson_tkn_pos;

typedef enum
{
  TOKEN_EOF = -1,
  TOKEN_INVALID = -2,

  TOKEN_STRING = -3,
  TOKEN_NUMBER = -4,
  TOKEN_NULL = -5,
  TOKEN_TRUE = -6,
  TOKEN_FALSE = -7,

  TOKEN_COLON = ':',
  TOKEN_COMMA = ',',
  TOKEN_LBRACE = '{',
  TOKEN_RBRACE = '}',
  TOKEN_LPAREN = '[',
  TOKEN_RPAREN = ']',
} jjson_tkn_type;

typedef struct
{
  jjson_tkn_type type;
  union
  {
    char *string;
    char chr;
    long number;
    unsigned int boolean;
  } label;
  jjson_tkn_pos pos;
} jjson_token;

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
} jjson_lexer;

typedef struct
{
  jjson_lexer lexer;
  jjson_token curr_token;
  jjson_token next_token;
} jjson_parser;

typedef struct
{
  FILE *stream;
  size_t tab;
  size_t tab_rate;
} jjson_stringfier;

jjson_lexer lexer_new(char *content);
void lexer_next_token(jjson_lexer *l, jjson_token *tkn);

enum jjson_error jjson_init(jjson_t *json)
{
  json->field_count = 0;
  json->capacity = JSON_CAPACITY_INCR_RATE;
  json->fields = (jjson_key_value *)malloc(sizeof(jjson_key_value) * JSON_CAPACITY_INCR_RATE);
  return JJE_OK;
}

enum jjson_error jjson_get(jjson_t *json, char *key, jjson_value **out)
{
  for (size_t i = 0; i < json->field_count; ++i)
  {
    jjson_key_value *tmp = &json->fields[i];
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
  jjson_value *value = NULL;

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
  jjson_value *value = NULL;

  enum jjson_error error = jjson_get(json, key, &value);
  if (JJE_OK != error || JSON_NUMBER != value->type)
  {
    return error;
  }

  *out = &value->data.number;
  return JJE_OK;
}

enum jjson_error jjson_add(jjson_t *json, jjson_key_value kv)
{
  if (json->capacity <= json->field_count)
  {
    size_t new_cap = json->capacity + JSON_CAPACITY_INCR_RATE;
    json->fields = (jjson_key_value *)realloc(json->fields, sizeof(jjson_key_value) * new_cap);
    json->capacity = new_cap;
  }
  json->fields[json->field_count++] = kv;
  return JJE_OK;
}

enum jjson_error jjson_add_string(jjson_t *json, char *key, char *value)
{
  jjson_key_value kv = {.key = key, .value.type = JSON_STRING, .value.data.string = value};
  return jjson_add(json, kv);
}

enum jjson_error jjson_add_number(jjson_t *json, char *key, long long value)
{
  jjson_key_value kv = {.key = key, .value.type = JSON_NUMBER, .value.data.number = value};
  return jjson_add(json, kv);
}

#define LEXER_EOF '\0'
typedef int (*lexer_read_predicate)(int);

void lexer_advance_one(jjson_lexer *l);
void lexer_advance_while(jjson_lexer *l, lexer_read_predicate pred);
void lexer_read_while(jjson_lexer *l, lexer_read_predicate pred, char **out);
void lexer_skip_whitespace(jjson_lexer *l);

jjson_lexer lexer_new(char *content)
{
  jjson_lexer l = {0};
  l.content = content;
  l.content_len = strlen(content);
  l.line = 1;
  lexer_advance_one(&l);
  return l;
}

void lexer_advance_one(jjson_lexer *l)
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

void lexer_advance_while(jjson_lexer *l, lexer_read_predicate pred)
{
  while (l->curr_char != LEXER_EOF && pred(l->curr_char))
  {
    lexer_advance_one(l);
  }
}

void lexer_read_while(jjson_lexer *l, lexer_read_predicate pred, char **out)
{
  size_t start = l->pos;
  while (l->curr_char != LEXER_EOF && pred(l->curr_char))
  {
    lexer_advance_one(l);
  }
  size_t buf_len = l->pos - start;
  *out = (char *)malloc(sizeof(char) * (buf_len + 1));
  strncpy(*out, l->content + start, buf_len);
  (*out)[buf_len] = '\0';
}

void lexer_skip_whitespace(jjson_lexer *l) { lexer_advance_while(l, isspace); }

int is_not_unquote(int c) { return c != '"'; }

void lexer_next_token(jjson_lexer *l, jjson_token *token)
{
  lexer_skip_whitespace(l);

  token->pos.line = l->line;
  token->pos.colm = l->colm;

  if (l->curr_char == LEXER_EOF)
  {
    token->type = TOKEN_EOF;
    return;
  }

  switch (l->curr_char)
  {
  case '{':
    lexer_advance_one(l);
    token->type = TOKEN_LBRACE;
    return;
  case '}':
    lexer_advance_one(l);
    token->type = TOKEN_RBRACE;
    return;
  case '[':
    lexer_advance_one(l);
    token->type = TOKEN_LPAREN;
    return;
  case ']':
    lexer_advance_one(l);
    token->type = TOKEN_RPAREN;
    return;
  case ':':
    lexer_advance_one(l);
    token->type = TOKEN_COLON;
    return;
  case ',':
    lexer_advance_one(l);
    token->type = TOKEN_COMMA;
    return;
  case '"':
    lexer_advance_one(l);
    token->type = TOKEN_STRING;
    lexer_read_while(l, is_not_unquote, &(token->label.string));
    lexer_advance_one(l);
    return;
  }

  if (isalpha(l->curr_char))
  {
    char *buf;
    lexer_read_while(l, isalpha, &buf);

    if (strcmp(buf, "null") == 0)
    {
      token->type = TOKEN_NULL;
      return;
    }

    if (strcmp(buf, "true") == 0)
    {
      token->type = TOKEN_TRUE;
      return;
    }

    if (strcmp(buf, "false") == 0)
    {
      token->type = TOKEN_FALSE;
      return;
    }

    token->type = TOKEN_INVALID;
    token->label.chr = buf[0];
    return;
  }

  // parsing `+69` as `69`
  if (l->curr_char == '+' && isdigit(l->content[l->read_pos]))
  {
    lexer_advance_one(l);
  }

  if (isdigit(l->curr_char) ||
      ((l->curr_char == '-') && ((l->content_len > l->read_pos) && isdigit(l->content[l->read_pos]))))
  {
    int is_signed = 0;
    if (l->curr_char == '-')
    {
      lexer_advance_one(l);
      is_signed = 1;
    }

    char *buf;
    lexer_read_while(l, isdigit, &buf);
    token->type = TOKEN_NUMBER;
    token->label.number = strtol(buf, NULL, 0);

    if (is_signed)
    {
      token->label.number -= (token->label.number * 2);
    }

    free(buf);
    return;
  }

  token->type = TOKEN_INVALID;
  token->label.chr = l->curr_char;
  lexer_advance_one(l);
  return;
}

void parser_bump(jjson_parser *p);
void parser_expect(jjson_parser *p, jjson_tkn_type tt);

// Parsers
enum jjson_error parse_json_object(jjson_parser *p, jjson_t *json);
jjson_value parse_json_value(jjson_parser *p);
jjson_array parse_json_array(jjson_parser *p);
jjson_key_value parse_json_key_value(jjson_parser *p);

enum jjson_error jjson_parse(jjson_t *json, char *raw)
{
  jjson_parser p = {.lexer = lexer_new(raw)};
  parser_bump(&p);
  parser_bump(&p);
  return parse_json_object(&p, json);
}

enum jjson_error parse_json_object(jjson_parser *p, jjson_t *json)
{
  if (p->curr_token.type == TOKEN_EOF)
  {
    return JJE_OK;
  }
  parser_expect(p, TOKEN_LBRACE);
  if (p->curr_token.type == TOKEN_RBRACE)
  {
    return JJE_OK;
  }
  while (1)
  {
    jjson_key_value pair = parse_json_key_value(p);
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

jjson_key_value parse_json_key_value(jjson_parser *p)
{
  jjson_key_value pair;
  if (p->curr_token.type != TOKEN_STRING)
  {
    printf("%s\n", TOKEN_TYPE(p->curr_token.type));
    fprintf(stderr, "[JSON ERROR]: Expected JSON key to be string at %lu:%lu\n", p->curr_token.pos.line,
            p->curr_token.pos.colm);
    exit(1);
  }
  pair.key = p->curr_token.label.string;
  parser_bump(p);
  parser_expect(p, TOKEN_COLON);
  pair.value = parse_json_value(p);
  return pair;
}

jjson_value parse_json_value(jjson_parser *p)
{
  jjson_value value;
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

jjson_array JsonArray_New()
{
  jjson_array array;
  array.length = 0;
  array.capacity = JSON_CAPACITY_INCR_RATE;
  array.items = (jjson_value *)malloc(sizeof(jjson_t) * JSON_CAPACITY_INCR_RATE);
  return array;
}

void JsonArray_Append(jjson_array *array, jjson_value val)
{
  if (array->capacity >= array->length)
  {
    size_t new_cap = array->capacity + JSON_CAPACITY_INCR_RATE;
    array->items = (jjson_value *)realloc(array->items, sizeof(jjson_value) * new_cap);
    array->capacity = new_cap;
  }
  array->items[array->length++] = val;
}

jjson_array parse_json_array(jjson_parser *p)
{
  parser_expect(p, TOKEN_LPAREN);
  jjson_array array = JsonArray_New();
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

void parser_bump(jjson_parser *p)
{
  p->curr_token = p->next_token;
  jjson_token tkn;
  lexer_next_token(&p->lexer, &tkn);
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

void parser_expect(jjson_parser *p, jjson_tkn_type tt)
{
  if (p->curr_token.type != tt)
  {
    fprintf(stderr, "[JSON ERROR]: Expected '%s' but got '%s' at %lu:%lu\n", TOKEN_TYPE(tt),
            TOKEN_TYPE(p->curr_token.type), p->curr_token.pos.line, p->curr_token.pos.colm);
    exit(1);
  }
  parser_bump(p);
}

void stringify_json_object(jjson_stringfier *ctx, jjson_t *obj);
void stringify_json_value(jjson_stringfier *ctx, jjson_value val);
void stringify_json_array(jjson_stringfier *ctx, jjson_array arr);
void stringfier_print_tab(jjson_stringfier *ctx);

enum jjson_error jjson_stringify(jjson_t *obj, short depth, char **out)
{
  unsigned long buf_len = 0;
  jjson_stringfier ctx;
  ctx.tab = depth;
  ctx.tab_rate = depth;
  ctx.stream = open_memstream(out, &buf_len);
  stringify_json_object(&ctx, obj);
  fclose(ctx.stream);
  return JJE_OK;
}

void stringify_json_object(jjson_stringfier *ctx, jjson_t *obj)
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

void stringify_json_value(jjson_stringfier *ctx, jjson_value val)
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

void stringify_json_array(jjson_stringfier *ctx, jjson_array arr)
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

void stringfier_print_tab(jjson_stringfier *ctx)
{
  for (unsigned long i = 0; i < ctx->tab; ++i)
  {
    fprintf(ctx->stream, " ");
  }
}

void jjson_dump(jjson_t *json, FILE *f, int depth)
{
  char *buf;
  jjson_stringify(json, depth, &buf);
  fprintf(f, "%s", buf);
  free(buf);
}
#endif // JACK_IMPLEMENTATION

#endif // __JACK_JSON_PARSER__
