#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

enum token {TK_NON = 0x0, TK_IDENT = 0x1, TK_KEYW = 0x2, TK_INT = 0x4, TK_FLOAT = 0x8, TK_CHAR = 0x10, TK_STRING = 0x20, TK_OP = 0x40, TK_PUNC = 0x80};

struct token_node {
	union {
		struct {
			void* data;
			size_t size; 
		} max;
		struct {
			char char_val; 
		};
		struct {
			int int_val; 
		};
	};
	enum token type;
	struct node *next;
};

struct context {
	struct vector* tokens;
};

struct vector {
	void* buff;
	size_t len;
	size_t cap;
	size_t ele_size;
	void* next;
};

struct parse_node {
};

void vector_init(struct vector **vec, size_t ele_size, size_t cap);
void vector_destroy(struct vector *v);
void vector_push_back(struct vector *v, void* val);
void* vector_next(struct vector *v);
void* vector_peek(struct vector *v);
void vector_reset(struct vector *v);
void* vector_back(struct vector *v);

int scan(struct context *ctx, FILE *fp);
int parse(struct context *ctx);
