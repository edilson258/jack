#ifndef JACK_JSON_PARSER
#define JACK_JSON_PARSER

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

typedef unsigned long usize;

typedef enum JsonType {
  JSON_STRING = 1,
  JSON_NUMBER = 2,
  JSON_ARRAY = 3,
  JSON_OBJECT = 4,
} JsonType;

typedef struct JsonArray {
  usize length;
  usize capacity;
  struct JsonValue *entries;
} JsonArray;

typedef struct JsonValue {
  JsonType type;
  union {
    long number;
    char *string;
    JsonArray array;
    struct Json *object;
  } data;
} JsonValue;

typedef struct JsonKeyValuePair {
  char *key;
  JsonValue value;
} JsonKeyValuePair;

typedef struct Json {
  usize capacity;
  usize entries_count;
  JsonKeyValuePair *entries;
} Json;

Json Json_New();
Json Json_Parse(char *str);
JsonKeyValuePair *Json_Get(Json *j, char *key);
void Json_Append(Json *j, JsonKeyValuePair pair);
char *Json_Stringfy(Json obj, unsigned int depth);

/*
 * INTERNAL API IMPLEMENTATION
 */

#ifdef JACK_IMPLEMENTATION

#define JSON_CAPACITY_INCR_RATE 25

typedef struct {
  unsigned long line;
  unsigned long colm;
} TokenPosition;

typedef enum {
  TOKEN_EOF = 1,
  TOKEN_INVALID = 2,

  TOKEN_STRING = 3,
  TOKEN_NUMBER = 4,

  TOKEN_COLON = ':',
  TOKEN_COMMA = ',',
  TOKEN_LBRACE = '{',
  TOKEN_RBRACE = '}',
  TOKEN_LPAREN = '[',
  TOKEN_RPAREN = ']',
} TokenType;

typedef struct {
  TokenType type;
  union {
    char *string;
    char chr;
    long number;
  } label;
  TokenPosition pos;
} Token;

#define TOKEN_TYPE(tt)                                                         \
  ((tt) == TOKEN_EOF       ? "EOF"                                             \
   : (tt) == TOKEN_INVALID ? "Invalid JSON token"                              \
   : (tt) == TOKEN_STRING  ? "String"                                          \
   : (tt) == TOKEN_NUMBER  ? "Number"                                          \
   : (tt) == TOKEN_COMMA   ? ","                                               \
   : (tt) == TOKEN_COLON   ? ":"                                               \
   : (tt) == TOKEN_LBRACE  ? "{"                                               \
   : (tt) == TOKEN_RBRACE  ? "}"                                               \
   : (tt) == TOKEN_LPAREN  ? "["                                               \
   : (tt) == TOKEN_RPAREN  ? "]"                                               \
                           : "Unkown JSON type")

typedef struct {
  usize line;
  usize colm;

  char *content;
  char curr_char;
  usize content_len;

  usize pos;
  usize read_pos;
} Lexer;

typedef struct {
  Lexer m_Lexer;
  Token m_CurrToken;
  Token m_NextToken;
} Parser;

typedef struct JsonStringfier {
  FILE *m_Stream;
  usize m_Tab;
  usize m_TabRate;
} JsonStringfier;

Lexer lexer_new(char *content);
Token lexer_next_token(Lexer *l);

void json_alloc_more_space(Json *j);

Json Json_New() {
  Json json;
  json.entries_count = 0;
  json.capacity = JSON_CAPACITY_INCR_RATE;
  json.entries =
      (JsonKeyValuePair *)malloc(sizeof(Json) * JSON_CAPACITY_INCR_RATE);
  return json;
}

JsonKeyValuePair *Json_Get(Json *j, char *key) {
  for (usize i = 0; i < j->entries_count; ++i) {
    JsonKeyValuePair *tmp = &j->entries[i];
    if (strcmp(key, tmp->key) == 0) {
      return tmp;
    }
  }
  return NULL;
}

void Json_Append(Json *j, JsonKeyValuePair pair) {
  if (j->capacity <= j->entries_count) {
    json_alloc_more_space(j);
  }
  j->entries[j->entries_count++] = pair;
}

void json_alloc_more_space(Json *j) {
  j->entries = (JsonKeyValuePair *)realloc(
      j->entries, j->capacity + JSON_CAPACITY_INCR_RATE);
  j->capacity += JSON_CAPACITY_INCR_RATE;
}

#define LEXER_EOF '\0'
typedef int (*lexer_read_predicate)(int);

void lexer_read_char(Lexer *l);
void lexer_drop_while(Lexer *l, lexer_read_predicate pred);
char *lexer_read_upto(Lexer *l, char stop_char);
void lexer_skip_whitespace(Lexer *l);

Lexer lexer_new(char *content) {
  Lexer l = {0};
  l.content = content;
  l.content_len = strlen(content);
  l.line = 1;
  lexer_read_char(&l);
  return l;
}

void lexer_read_char(Lexer *l) {
  if (l->read_pos >= l->content_len) {
    l->curr_char = LEXER_EOF;
    l->pos = l->read_pos;
    return;
  }

  l->curr_char = l->content[l->read_pos];
  l->pos = l->read_pos;
  l->read_pos += 1;

  if (l->curr_char == '\n') {
    l->line += 1;
    l->colm = 0;
  } else {
    l->colm += 1;
  }
}

void lexer_drop_while(Lexer *l, lexer_read_predicate pred) {
  while (l->curr_char != LEXER_EOF && pred(l->curr_char)) {
    lexer_read_char(l);
  }
}

char *lexer_read_while(Lexer *l, lexer_read_predicate pred) {
  usize start = l->pos;
  while (l->curr_char != LEXER_EOF && pred(l->curr_char)) {
    lexer_read_char(l);
  }
  usize buf_len = l->pos - start;
  char *buf = (char *)malloc(sizeof(char) * (buf_len + 1));
  strncpy(buf, l->content + start, buf_len);
  buf[buf_len] = '\0';
  return buf;
}

void lexer_skip_whitespace(Lexer *l) { lexer_drop_while(l, isspace); }

int is_not_unquote(int c) { return c != '"'; }

Token lexer_next_token(Lexer *l) {
  lexer_skip_whitespace(l);

  Token token;
  token.pos.line = l->line;
  token.pos.colm = l->colm;

  if (l->curr_char == LEXER_EOF) {
    token.type = TOKEN_EOF;
    return token;
  }

  switch (l->curr_char) {
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

  if (isdigit(l->curr_char)) {
    char *buf = lexer_read_while(l, isdigit);
    token.type = TOKEN_NUMBER;
    token.label.number = strtol(buf, NULL, 0);
    free(buf);
    return token;
  }

  token.type = TOKEN_INVALID;
  token.label.chr = l->curr_char;
  lexer_read_char(l);
  return token;
}

void parser_bump(Parser *p);
void parser_bump_expected(Parser *p, TokenType tt);

// Parsers
Json parse_json_object(Parser *p);
JsonValue parse_json_value(Parser *p);
JsonArray parse_json_array(Parser *p);
JsonKeyValuePair parse_json_key_value(Parser *p);

Json Json_Parse(char *str) {
  Parser p = {.m_Lexer = lexer_new(str)};
  parser_bump(&p);
  parser_bump(&p);
  return parse_json_object(&p);
}

Json parse_json_object(Parser *p) {
  Json json = Json_New();
  if (p->m_CurrToken.type == TOKEN_EOF) {
    return json;
  }
  parser_bump_expected(p, TOKEN_LBRACE);
  if (p->m_CurrToken.type == TOKEN_RBRACE) {
    return json;
  }
  while (1) {
    JsonKeyValuePair pair = parse_json_key_value(p);
    Json_Append(&json, pair);
    if (p->m_CurrToken.type == TOKEN_COMMA) {
      parser_bump(p);
    }
    if (p->m_CurrToken.type == TOKEN_RBRACE) {
      break;
    }
    if (p->m_CurrToken.type == TOKEN_EOF) {
      break;
    }
  }
  return json;
}

JsonKeyValuePair parse_json_key_value(Parser *p) {
  JsonKeyValuePair pair;
  if (p->m_CurrToken.type != TOKEN_STRING) {
    printf("%s\n", TOKEN_TYPE(p->m_CurrToken.type));
    fprintf(stderr, "[JSON ERROR]: Expected JSON key to be string at %lu:%lu\n",
            p->m_CurrToken.pos.line, p->m_CurrToken.pos.colm);
    exit(1);
  }
  pair.key = p->m_CurrToken.label.string;
  parser_bump(p);
  parser_bump_expected(p, TOKEN_COLON);
  pair.value = parse_json_value(p);
  return pair;
}

JsonValue parse_json_value(Parser *p) {
  JsonValue value;
  switch (p->m_CurrToken.type) {
  case TOKEN_NUMBER:
    value.type = JSON_NUMBER;
    value.data.number = p->m_CurrToken.label.number;
    break;
  case TOKEN_STRING:
    value.type = JSON_STRING;
    value.data.string = p->m_CurrToken.label.string;
    break;
  case TOKEN_LPAREN:
    value.type = JSON_ARRAY;
    value.data.array = parse_json_array(p);
    break;
  case TOKEN_LBRACE:
    value.type = JSON_OBJECT;
    value.data.object = (Json *)malloc(sizeof(Json));
    *value.data.object = parse_json_object(p);
    break;
  default:
    fprintf(stderr, "[JSON ERROR]: Unsupported JSON value at %lu:%lu\n",
            p->m_CurrToken.pos.line, p->m_CurrToken.pos.colm);
    exit(1);
  }
  parser_bump(p);
  return value;
}

JsonArray JsonArray_New() {
  JsonArray array;
  array.length = 0;
  array.capacity = JSON_CAPACITY_INCR_RATE;
  array.entries = (JsonValue *)malloc(sizeof(Json) * JSON_CAPACITY_INCR_RATE);
  return array;
}

void JsonArray_Append(JsonArray *array, JsonValue val) {
  if (array->capacity >= array->length) {
  }
  array->entries[array->length++] = val;
}

JsonArray parse_json_array(Parser *p) {
  parser_bump_expected(p, TOKEN_LPAREN);
  JsonArray array = JsonArray_New();
  while (p->m_CurrToken.type != TOKEN_EOF &&
         p->m_CurrToken.type != TOKEN_RPAREN) {
    JsonArray_Append(&array, parse_json_value(p));
    if (p->m_CurrToken.type == TOKEN_RPAREN) {
      break;
    }
    if (p->m_CurrToken.type == TOKEN_COMMA) {
      parser_bump(p);
    } else {
      fprintf(stderr,
              "[JSON ERROR]: Expected ',' to separated Json Array items at "
              "%lu:%lu\n",
              p->m_CurrToken.pos.line, p->m_CurrToken.pos.colm);
      exit(1);
    }
  }

  return array;
}

void parser_bump(Parser *p) {
  p->m_CurrToken = p->m_NextToken;
  Token tkn = lexer_next_token(&p->m_Lexer);
  switch (tkn.type) {
  case TOKEN_INVALID:
    fprintf(stderr, "[JSON ERROR]: Invalid symbol '%c' at %lu:%lu\n",
            tkn.label.chr, tkn.pos.line, tkn.pos.colm);
    exit(1);
  default:
    p->m_NextToken = tkn;
    break;
  }
}

void parser_bump_expected(Parser *p, TokenType tt) {
  if (p->m_CurrToken.type != tt) {
    fprintf(stderr, "[JSON ERROR]: Expected '%s' but got '%s' at %lu:%lu\n",
            TOKEN_TYPE(tt), TOKEN_TYPE(p->m_CurrToken.type),
            p->m_CurrToken.pos.line, p->m_CurrToken.pos.colm);
    exit(1);
  }
  parser_bump(p);
}

void stringfy_json_object(JsonStringfier *ctx, Json obj);
void stringfy_json_value(JsonStringfier *ctx, JsonValue val);
void stringfy_json_array(JsonStringfier *ctx, JsonArray arr);
void stringfier_print_tab(JsonStringfier *ctx);

char *Json_Stringfy(Json obj, unsigned int depth) {
  char *buf = NULL;
  unsigned long buf_len = 0;
  JsonStringfier ctx;
  ctx.m_TabRate = depth;
  ctx.m_Tab = depth;
  ctx.m_Stream = open_memstream(&buf, &buf_len);
  stringfy_json_object(&ctx, obj);
  fclose(ctx.m_Stream);
  return buf;
}

void stringfy_json_object(JsonStringfier *ctx, Json obj) {
  fprintf(ctx->m_Stream, "{\n");
  for (unsigned long i = 0; i < obj.entries_count; ++i) {
    stringfier_print_tab(ctx);
    fprintf(ctx->m_Stream, "\"%s\": ", obj.entries[i].key);
    stringfy_json_value(ctx, obj.entries[i].value);
    if (i + 1 < obj.entries_count) {
      fprintf(ctx->m_Stream, ",");
    }
    fprintf(ctx->m_Stream, "\n");
  }
  ctx->m_Tab == ctx->m_TabRate ? fprintf(ctx->m_Stream, "}\n") : ({
    ctx->m_Tab -= ctx->m_TabRate;
    stringfier_print_tab(ctx);
    fprintf(ctx->m_Stream, "}");
    ctx->m_Tab += ctx->m_TabRate;
  });
}

void stringfy_json_value(JsonStringfier *ctx, JsonValue val) {
  switch (val.type) {
  case JSON_NUMBER:
    fprintf(ctx->m_Stream, "%lu", val.data.number);
    return;
  case JSON_STRING:
    fprintf(ctx->m_Stream, "\"%s\"", val.data.string);
    return;
  case JSON_ARRAY:
    stringfy_json_array(ctx, val.data.array);
    return;
  case JSON_OBJECT:
    ctx->m_Tab += ctx->m_TabRate;
    stringfy_json_object(ctx, *val.data.object);
    ctx->m_Tab -= ctx->m_TabRate;
    return;
  }
}

void stringfy_json_array(JsonStringfier *ctx, JsonArray arr) {
  fprintf(ctx->m_Stream, "[");
  ctx->m_Tab += ctx->m_TabRate;
  for (int i = 0; i < arr.length; ++i) {
    fprintf(ctx->m_Stream, "\n");
    stringfier_print_tab(ctx);
    stringfy_json_value(ctx, arr.entries[i]);
    if (i + 1 < arr.length) {
      fprintf(ctx->m_Stream, ",");
    }
  }
  ctx->m_Tab -= ctx->m_TabRate;
  fprintf(ctx->m_Stream, "\n");
  stringfier_print_tab(ctx);
  fprintf(ctx->m_Stream, "]");
}

void stringfier_print_tab(JsonStringfier *ctx) {
  for (unsigned long i = 0; i < ctx->m_Tab; ++i) {
    fprintf(ctx->m_Stream, " ");
  }
}
#endif // JACK_IMPLEMENTATION

#endif // JACK_JSON_PARSER
