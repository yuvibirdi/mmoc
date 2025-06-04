#ifndef LEX_H
#define LEX_H
#define EoF 256
#define DIGIT 257

typedef struct Token_type {
    int class;
    char repr;
} Token_type;
extern Token_type type;
extern void get_next_token(void);
#endif
