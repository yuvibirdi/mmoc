#include "lex.h"
#include "stdio.h"
#define EoF 256
#define DIGIT 257
/* PRIVATE */
static int Is_layout_char(int ch) {
  switch (ch) {
  case ' ':
  case '\t':
  case '\n':
    return 1;
  default:
    return 0;
  }
}
/* PUBLIC */
Token_type Token;
void get_next_token(void) {
  int ch;
  /* get a non−layout character: */
  do {
    ch = getchar();
    if (ch < 0) {
      Token.class = EoF;
      Token.repr = '#' ;
      return;
    }
  } while (Is_layout_char(ch));
  /* classify it : */
  if ( '0' <= ch && ch <= '9') {
    Token.class = DIGIT;
  } else {
    Token.class = ch;
  }
  Token.repr = ch;
}
