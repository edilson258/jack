#ifndef __JACK_JSON_PARSER__
#define __JACK_JSON_PARSER__

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define JJSON_GET_NUMBER(json, key, varname) \
  signed long long *varname;                 \
  jjson_get_number(&json, key, &varname)

#define JJSON_GET_STRING(json, key, varname) \
  char *varname;                             \
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
  const char *key;
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
  JJE_INVALID_TKN = -3,
};

enum jjson_error jjson_init(jjson_t *json);
enum jjson_error jjson_init_array(jjson_array *arr);

enum jjson_error jjson_parse(jjson_t *json, char *raw);
enum jjson_error jjson_stringify(const jjson_t *obj, short depth, char **out);

enum jjson_error jjson_get(jjson_t *json, const char *key, jjson_value **out);
enum jjson_error jjson_get_string(jjson_t *json, const char *key, char **out);
enum jjson_error jjson_get_number(jjson_t *json, const char *key, long long **out);

enum jjson_error jjson_add(jjson_t *json, jjson_key_value kv);
enum jjson_error jjson_add_string(jjson_t *json, const char *key, const char *value);
enum jjson_error jjson_add_number(jjson_t *json, const char *key, const long long value);
enum jjson_error jjson_array_push(jjson_array *array, jjson_value val);

enum jjson_error jjson_deinit(jjson_t *json);
enum jjson_error jjson_deinit_object(jjson_t *json);
enum jjson_error jjson_deinit_array(jjson_array *arr);
enum jjson_error jjson_deinit_value(jjson_value *val);

char *jjson_strerror();
void jjson_dump(const jjson_t *json, FILE *f, int depth);

#ifdef JACK_IMPLEMENTATION

#define JJSON__MAX_STR_LEN (5 * 1024)
#define JJSON__ERROR_MSG_MAX_LEN 1024
char jjson__last_error_message[JJSON__ERROR_MSG_MAX_LEN];
char *jjson_strerror() { return jjson__last_error_message; }

#define JSON_CAPACITY_INCR_RATE 256

typedef struct
{
  unsigned long line;
  unsigned long colm;
} jjson__tkn_pos;

typedef enum
{
  JJSON__TOKEN_EOF = -1,
  JJSON__TOKEN_INVALID = -2,

  JJSON__TOKEN_STRING = -3,
  JJSON__TOKEN_NUMBER = -4,
  JJSON__TOKEN_NULL = -5,
  JJSON__TOKEN_TRUE = -6,
  JJSON__TOKEN_FALSE = -7,

  JJSON__TOKEN_COLON = ':',
  JJSON__TOKEN_COMMA = ',',
  JJSON__TOKEN_LBRACE = '{',
  JJSON__TOKEN_RBRACE = '}',
  JJSON__TOKEN_LPAREN = '[',
  JJSON__TOKEN_RPAREN = ']',
} jjson__tkn_type;

typedef struct
{
  jjson__tkn_type type;
  union
  {
    char *string;
    char chr;
    long number;
    unsigned int boolean;
  } label;
  jjson__tkn_pos pos;
} jjson__token;

#define JJSON__TOKEN_TYPE(tt)                            \
  ((tt) == JJSON__TOKEN_EOF       ? "EOF"                \
   : (tt) == JJSON__TOKEN_INVALID ? "Invalid JSON token" \
   : (tt) == JJSON__TOKEN_STRING  ? "String"             \
   : (tt) == JJSON__TOKEN_NUMBER  ? "Number"             \
   : (tt) == JJSON__TOKEN_COMMA   ? ","                  \
   : (tt) == JJSON__TOKEN_COLON   ? ":"                  \
   : (tt) == JJSON__TOKEN_LBRACE  ? "{"                  \
   : (tt) == JJSON__TOKEN_RBRACE  ? "}"                  \
   : (tt) == JJSON__TOKEN_LPAREN  ? "["                  \
   : (tt) == JJSON__TOKEN_RPAREN  ? "]"                  \
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
} jjson__lexer;

typedef struct
{
  jjson__lexer lexer;
  jjson__token curr_token;
  jjson__token next_token;
} jjson__parser;

typedef struct
{
  FILE *stream;
  size_t tab;
  size_t tab_rate;
} jjson__stringfier;

void jjson__lexer_init(jjson__lexer *l, char *content);
void jjson__lexer_next_token(jjson__lexer *l, jjson__token *tkn);

enum jjson_error jjson_init(jjson_t *json)
{
  json->field_count = 0;
  json->capacity = JSON_CAPACITY_INCR_RATE;
  json->fields = (jjson_key_value *)malloc(sizeof(jjson_key_value) * JSON_CAPACITY_INCR_RATE);
  return JJE_OK;
}

enum jjson_error jjson_get(jjson_t *json, const char *key, jjson_value **out)
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

enum jjson_error jjson_get_string(jjson_t *json, const char *key, char **out)
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

enum jjson_error jjson_get_number(jjson_t *json, const char *key, long long **out)
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

enum jjson_error jjson_add_string(jjson_t *json, const char *key, const char *value)
{
  jjson_key_value kv = {.key = strndup(key, JJSON__MAX_STR_LEN), .value.type = JSON_STRING, .value.data.string = strndup(value, JJSON__MAX_STR_LEN)};
  return jjson_add(json, kv);
}

enum jjson_error jjson_add_number(jjson_t *json, const char *key, long long value)
{
  jjson_key_value kv = {.key = strndup(key, JJSON__MAX_STR_LEN), .value.type = JSON_NUMBER, .value.data.number = value};
  return jjson_add(json, kv);
}

/**
 * JSON Lexer
 */

#define LEXER_EOF '\0'
typedef int (*lexer_read_predicate)(int);

void jjson__lexer_advance_one(jjson__lexer *l);
void jjson__lexer_advance_while(jjson__lexer *l, lexer_read_predicate pred);
void jjson__lexer_read_while(jjson__lexer *l, lexer_read_predicate pred, char **out);
void jjson__lexer_skip_whitespace(jjson__lexer *l);

void jjson__lexer_init(jjson__lexer *l, char *content)
{
  l->content = content;
  l->content_len = strlen(content);
  l->line = 1;
  jjson__lexer_advance_one(l);
}

void jjson__lexer_advance_one(jjson__lexer *l)
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

void jjson__lexer_advance_while(jjson__lexer *l, lexer_read_predicate pred)
{
  while (l->curr_char != LEXER_EOF && pred(l->curr_char))
  {
    jjson__lexer_advance_one(l);
  }
}

void jjson__lexer_read_while(jjson__lexer *l, lexer_read_predicate pred, char **out)
{
  size_t start = l->pos;
  while (l->curr_char != LEXER_EOF && pred(l->curr_char))
  {
    jjson__lexer_advance_one(l);
  }
  size_t buf_len = l->pos - start;
  *out = (char *)malloc(sizeof(char) * (buf_len + 1));
  strncpy(*out, l->content + start, buf_len);
  (*out)[buf_len] = '\0';
}

void jjson__lexer_skip_whitespace(jjson__lexer *l)
{
  jjson__lexer_advance_while(l, isspace);
}

int jjson__lexer_is_not_unquote(int c) { return c != '"'; }

void jjson__lexer_next_token(jjson__lexer *l, jjson__token *token)
{
  jjson__lexer_skip_whitespace(l);

  token->pos.line = l->line;
  token->pos.colm = l->colm;

  if (l->curr_char == LEXER_EOF)
  {
    token->type = JJSON__TOKEN_EOF;
    return;
  }

  switch (l->curr_char)
  {
  case '{':
    jjson__lexer_advance_one(l);
    token->type = JJSON__TOKEN_LBRACE;
    return;
  case '}':
    jjson__lexer_advance_one(l);
    token->type = JJSON__TOKEN_RBRACE;
    return;
  case '[':
    jjson__lexer_advance_one(l);
    token->type = JJSON__TOKEN_LPAREN;
    return;
  case ']':
    jjson__lexer_advance_one(l);
    token->type = JJSON__TOKEN_RPAREN;
    return;
  case ':':
    jjson__lexer_advance_one(l);
    token->type = JJSON__TOKEN_COLON;
    return;
  case ',':
    jjson__lexer_advance_one(l);
    token->type = JJSON__TOKEN_COMMA;
    return;
  case '"':
    jjson__lexer_advance_one(l);
    token->type = JJSON__TOKEN_STRING;
    jjson__lexer_read_while(l, jjson__lexer_is_not_unquote, &(token->label.string));
    jjson__lexer_advance_one(l);
    return;
  }

  if (isalpha(l->curr_char))
  {
    char *buf;
    jjson__lexer_read_while(l, isalpha, &buf);

    if (strcmp(buf, "null") == 0)
    {
      token->type = JJSON__TOKEN_NULL;
    }
    else if (strcmp(buf, "true") == 0)
    {
      token->type = JJSON__TOKEN_TRUE;
    }
    else if (strcmp(buf, "false") == 0)
    {
      token->type = JJSON__TOKEN_FALSE;
    }
    else
    {
      token->type = JJSON__TOKEN_INVALID;
      token->label.chr = buf[0];
    }

    free(buf);
    return;
  }

  // parsing `+69` as `69`
  if (l->curr_char == '+' && isdigit(l->content[l->read_pos]))
  {
    jjson__lexer_advance_one(l);
  }

  if (isdigit(l->curr_char) ||
      ((l->curr_char == '-') && ((l->content_len > l->read_pos) && isdigit(l->content[l->read_pos]))))
  {
    int is_signed = 0;
    if (l->curr_char == '-')
    {
      jjson__lexer_advance_one(l);
      is_signed = 1;
    }

    char *buf;
    jjson__lexer_read_while(l, isdigit, &buf);
    token->type = JJSON__TOKEN_NUMBER;
    token->label.number = strtol(buf, NULL, 0);

    if (is_signed)
    {
      token->label.number -= (token->label.number * 2);
    }

    free(buf);
    return;
  }

  token->type = JJSON__TOKEN_INVALID;
  token->label.chr = l->curr_char;
  jjson__lexer_advance_one(l);
  return;
}

/**
 * JSON Parser
 */

enum jjson_error jjson__parser_bump(jjson__parser *p);
enum jjson_error jjson__parser_expect(jjson__parser *p, jjson__tkn_type tt);
enum jjson_error jjson__parse_json_object(jjson__parser *p, jjson_t *json);
enum jjson_error jjson__parse_json_value(jjson__parser *p, jjson_value *val);
enum jjson_error jjson__parse_json_array(jjson__parser *p, jjson_array *arr);
enum jjson_error jjson__parse_json_key_value(jjson__parser *p, jjson_key_value *kv);

enum jjson_error jjson_parse(jjson_t *json, char *raw)
{
  jjson__parser p = {0};
  jjson__lexer_init(&p.lexer, raw);
  enum jjson_error err = jjson__parser_bump(&p);
  if (JJE_OK != err)
    return err;
  err = jjson__parser_bump(&p);
  if (JJE_OK != err)
    return err;
  return jjson__parse_json_object(&p, json);
}

enum jjson_error jjson__parse_json_object(jjson__parser *p, jjson_t *json)
{
  enum jjson_error err = JJE_OK;
  if (p->curr_token.type == JJSON__TOKEN_EOF)
  {
    // There is nothing to be parsed
    return JJE_OK;
  }
  err = jjson__parser_expect(p, JJSON__TOKEN_LBRACE);
  if (JJE_OK != err)
    return err;
  if (p->curr_token.type == JJSON__TOKEN_RBRACE)
  {
    return JJE_OK;
  }
  while (1)
  {
    jjson_key_value kv = {0};
    err = jjson__parse_json_key_value(p, &kv);
    if (JJE_OK != err)
      return err;
    err = jjson_add(json, kv);
    if (JJE_OK != err)
      return err;
    if (p->curr_token.type == JJSON__TOKEN_COMMA)
    {
      err = jjson__parser_bump(p);
      if (JJE_OK != err)
        return err;
    }
    if (p->curr_token.type == JJSON__TOKEN_RBRACE)
    {
      break;
    }
    if (p->curr_token.type == JJSON__TOKEN_EOF)
    {
      break;
    }
  }
  return JJE_OK;
}

enum jjson_error jjson__parse_json_key_value(jjson__parser *p, jjson_key_value *kv)
{
  enum jjson_error err = JJE_OK;
  if (p->curr_token.type != JJSON__TOKEN_STRING)
  {
    snprintf(jjson__last_error_message, JJSON__ERROR_MSG_MAX_LEN, "[JSON ERROR]: Expected JSON key to be string at %lu:%lu", p->curr_token.pos.line, p->curr_token.pos.colm);
    return JJE_INVALID_TKN;
  }
  kv->key = p->curr_token.label.string;
  err = jjson__parser_bump(p);
  if (JJE_OK != err)
    return err;
  err = jjson__parser_expect(p, JJSON__TOKEN_COLON);
  if (JJE_OK != err)
    return err;
  return jjson__parse_json_value(p, &kv->value);
}

enum jjson_error jjson__parse_json_value(jjson__parser *p, jjson_value *val)
{
  enum jjson_error err = JJE_OK;
  switch (p->curr_token.type)
  {
  case JJSON__TOKEN_NUMBER:
    val->type = JSON_NUMBER;
    val->data.number = p->curr_token.label.number;
    break;
  case JJSON__TOKEN_STRING:
    val->type = JSON_STRING;
    val->data.string = p->curr_token.label.string;
    break;
  case JJSON__TOKEN_LPAREN:
    val->type = JSON_ARRAY;
    err = jjson_init_array(&val->data.array);
    if (JJE_OK != err)
      return err;
    err = jjson__parse_json_array(p, &val->data.array);
    if (JJE_OK != err)
      return err;
    break;
  case JJSON__TOKEN_LBRACE:
    val->type = JSON_OBJECT;
    val->data.object = (jjson_t *)malloc(sizeof(jjson_t));
    err = jjson_init(val->data.object);
    if (JJE_OK != err)
      return err;
    err = jjson__parse_json_object(p, val->data.object);
    if (JJE_OK != err)
      return err;
    break;
  case JJSON__TOKEN_NULL:
    val->type = JSON_NULL;
    break;
  case JJSON__TOKEN_TRUE:
    val->type = JSON_BOOLEAN;
    val->data.boolean = JSON_TRUE;
    break;
  case JJSON__TOKEN_FALSE:
    val->type = JSON_BOOLEAN;
    val->data.boolean = JSON_FALSE;
    break;
  default:
    snprintf(jjson__last_error_message, JJSON__ERROR_MSG_MAX_LEN, "[JSON ERROR]: Unsupported JSON value at %lu:%lu", p->curr_token.pos.line, p->curr_token.pos.colm);
    return JJE_INVALID_TKN;
  }
  return jjson__parser_bump(p);
}

enum jjson_error jjson_init_array(jjson_array *arr)
{
  arr->length = 0;
  arr->capacity = JSON_CAPACITY_INCR_RATE;
  arr->items = (jjson_value *)malloc(sizeof(jjson_t) * JSON_CAPACITY_INCR_RATE);
  // TODO: check malloc result
  return JJE_OK;
}

enum jjson_error jjson_array_push(jjson_array *array, jjson_value val)
{
  if (array->capacity >= array->length)
  {
    size_t new_cap = array->capacity + JSON_CAPACITY_INCR_RATE;
    array->items = (jjson_value *)realloc(array->items, sizeof(jjson_value) * new_cap);
    // TODO: check malloc result
    array->capacity = new_cap;
  }
  array->items[array->length++] = val;
  return JJE_OK;
}

enum jjson_error jjson__parse_json_array(jjson__parser *p, jjson_array *arr)
{
  enum jjson_error err = JJE_OK;
  err = jjson__parser_expect(p, JJSON__TOKEN_LPAREN);
  if (JJE_OK != err)
    return err;
  while (p->curr_token.type != JJSON__TOKEN_EOF && p->curr_token.type != JJSON__TOKEN_RPAREN)
  {
    jjson_value val;
    err = jjson__parse_json_value(p, &val);
    if (JJE_OK != err)
      return err;
    err = jjson_array_push(arr, val);
    if (JJE_OK != err)
      return err;
    if (p->curr_token.type == JJSON__TOKEN_RPAREN)
    {
      break;
    }
    if (p->curr_token.type == JJSON__TOKEN_COMMA)
    {
      err = jjson__parser_bump(p);
      if (JJE_OK != err)
        return err;
    }
    else
    {
      snprintf(jjson__last_error_message, JJSON__ERROR_MSG_MAX_LEN, "[JSON ERROR]: Expected ',' to separated Json Array items at "
                                                                    "%lu:%lu",
               p->curr_token.pos.line, p->curr_token.pos.colm);
      return JJE_INVALID_TKN;
    }
  }
  return err;
}

enum jjson_error jjson__parser_bump(jjson__parser *p)
{
  p->curr_token = p->next_token;
  jjson__token tkn;
  jjson__lexer_next_token(&p->lexer, &tkn);
  switch (tkn.type)
  {
  case JJSON__TOKEN_INVALID:
    snprintf(jjson__last_error_message, JJSON__ERROR_MSG_MAX_LEN - 1, "[JSON ERROR]: Invalid symbol '%c' at %lu:%lu", tkn.label.chr, tkn.pos.line, tkn.pos.colm);
    return JJE_INVALID_TKN;
  default:
    p->next_token = tkn;
    break;
  }
  return JJE_OK;
}

enum jjson_error jjson__parser_expect(jjson__parser *p, jjson__tkn_type tt)
{
  if (p->curr_token.type != tt)
  {
    snprintf(jjson__last_error_message, JJSON__ERROR_MSG_MAX_LEN, "[JSON ERROR]: Expected '%s' but got '%s' at %lu:%lu", JJSON__TOKEN_TYPE(tt), JJSON__TOKEN_TYPE(p->curr_token.type), p->curr_token.pos.line, p->curr_token.pos.colm);
    return JJE_INVALID_TKN;
  }
  return jjson__parser_bump(p);
}

/**
 * JSON Stringifier
 */

void jjson__stringify_json_object(jjson__stringfier *ctx, const jjson_t *obj);
void jjson__stringify_json_value(jjson__stringfier *ctx, jjson_value val);
void jjson__stringify_json_array(jjson__stringfier *ctx, jjson_array arr);
void jjson__stringfier_print_tab(jjson__stringfier *ctx);

enum jjson_error jjson_stringify(const jjson_t *obj, short depth, char **out)
{
  unsigned long buf_len = 0;
  jjson__stringfier ctx;
  ctx.tab = depth;
  ctx.tab_rate = depth;
  ctx.stream = open_memstream(out, &buf_len);
  jjson__stringify_json_object(&ctx, obj);
  fclose(ctx.stream);
  return JJE_OK;
}

void jjson__stringify_json_object(jjson__stringfier *ctx, const jjson_t *obj)
{
  fprintf(ctx->stream, "{\n");
  for (unsigned long i = 0; i < obj->field_count; ++i)
  {
    jjson__stringfier_print_tab(ctx);
    fprintf(ctx->stream, "\"%s\": ", obj->fields[i].key);
    jjson__stringify_json_value(ctx, obj->fields[i].value);
    if (i + 1 < obj->field_count)
    {
      fprintf(ctx->stream, ",");
    }
    fprintf(ctx->stream, "\n");
  }
  ctx->tab == ctx->tab_rate ? fprintf(ctx->stream, "}\n") : ({
    ctx->tab -= ctx->tab_rate;
    jjson__stringfier_print_tab(ctx);
    fprintf(ctx->stream, "}");
    ctx->tab += ctx->tab_rate;
  });
}

void jjson__stringify_json_value(jjson__stringfier *ctx, jjson_value val)
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
    jjson__stringify_json_array(ctx, val.data.array);
    return;
  case JSON_OBJECT:
    ctx->tab += ctx->tab_rate;
    jjson__stringify_json_object(ctx, val.data.object);
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

void jjson__stringify_json_array(jjson__stringfier *ctx, jjson_array arr)
{
  fprintf(ctx->stream, "[");
  ctx->tab += ctx->tab_rate;
  for (int i = 0; i < arr.length; ++i)
  {
    fprintf(ctx->stream, "\n");
    jjson__stringfier_print_tab(ctx);
    jjson__stringify_json_value(ctx, arr.items[i]);
    if (i + 1 < arr.length)
    {
      fprintf(ctx->stream, ",");
    }
  }
  ctx->tab -= ctx->tab_rate;
  fprintf(ctx->stream, "\n");
  jjson__stringfier_print_tab(ctx);
  fprintf(ctx->stream, "]");
}

void jjson__stringfier_print_tab(jjson__stringfier *ctx)
{
  for (unsigned long i = 0; i < ctx->tab; ++i)
  {
    fprintf(ctx->stream, " ");
  }
}

void jjson_dump(const jjson_t *json, FILE *f, int depth)
{
  char *buf;
  jjson_stringify(json, depth, &buf);
  fprintf(f, "%s", buf);
  free(buf);
}

enum jjson_error jjson_deinit(jjson_t *json)
{
  return jjson_deinit_object(json);
}

enum jjson_error jjson_deinit_object(jjson_t *json)
{
  enum jjson_error err = JJE_OK;
  for (int i = 0; i < json->field_count; ++i)
  {
    jjson_key_value *kv = &json->fields[i];
    free((void *)kv->key);
    err = jjson_deinit_value(&kv->value);
    if (JJE_OK != err)
    {
      return err;
    }
  }
  free(json->fields);
  return JJE_OK;
}

enum jjson_error jjson_deinit_array(jjson_array *arr)
{
  enum jjson_error err = JJE_OK;
  for (int i = 0; i < arr->length; ++i)
  {
    err = jjson_deinit_value(&arr->items[i]);
    if (JJE_OK != err)
    {
      return err;
    }
  }
  free(arr->items);
  return err;
}

enum jjson_error jjson_deinit_value(jjson_value *val)
{
  enum jjson_error err = JJE_OK;
  switch (val->type)
  {
  case JSON_STRING:
    free(val->data.string);
    break;
  case JSON_OBJECT:
    err = jjson_deinit_object(val->data.object);
    free(val->data.object);
    break;
  case JSON_ARRAY:
    err = jjson_deinit_array(&val->data.array);
    break;
  default:
    break;
  }
  return err;
}
#endif // JACK_IMPLEMENTATION

#endif // __JACK_JSON_PARSER__
