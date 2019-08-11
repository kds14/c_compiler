#ifndef COMP_H
#define COMP_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define PAR_DBG

enum token {TK_NON = 0x0, TK_IDENT = 0x1, TK_KEYW = 0x2, TK_INT = 0x4,
			TK_FLOAT = 0x8, TK_CHAR = 0x10, TK_STRING = 0x20,
			TK_OP = 0x40, TK_PUNC = 0x80, TK_LPAREN = 0x100,
			TK_RPAREN = 0x200, TK_PARENS = 0x300, TK_TYPE = 0x400,
			TK_ASS = 0x800, TK_TEXT = (TK_IDENT | TK_KEYW | TK_TYPE),
			TK_ALL = 0xFFFFFFFF};

enum ast_type { AST_OP, AST_INT, AST_ASS, AST_VAR };

enum var_type { T_NON = 0, T_INT };

struct token_node {
	union {
		struct {
			void* data;
			size_t size; 
		};
		struct {
			char* str_val;
			size_t str_size; 
		};
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

void print_token(struct token_node *t);

struct ast_node {
	union {
		struct {
			char* ident;
			enum var_type vtype;
		};
		struct {
			void* data;
			size_t size; 
		};
		struct {
			char char_val; 
		};
		struct {
			int int_val; 
		};
	};
	enum ast_type type;
	struct ast_node *left;
	struct ast_node *right;
};


struct context {
	struct vector* tokens;
	struct ast_node* ast_root;
};

struct vector {
	void* buff;
	size_t len;
	size_t cap;
	size_t ele_size;
	void* next;
};

void vector_init(struct vector **vec, size_t ele_size, size_t cap);
void vector_destroy(struct vector *v);
void vector_push_back(struct vector *v, void* val);
void* vector_next(struct vector *v);
void* vector_peek(struct vector *v);
void vector_reset(struct vector *v);
void* vector_back(struct vector *v);

struct ht_node {
	char* key;
	void* val;
	struct ht_node *next;
};

struct hashtable {
	struct ht_node** eles;
	size_t buckets;
	size_t count;
	float bound;
	size_t it_bucket;
	struct ht_node* next;
};

void ht_init(struct hashtable **ht, size_t buckets, float bound);
struct ht_node* ht_next(struct hashtable *ht);
void ht_reset(struct hashtable *ht);
void ht_destroy(struct hashtable *ht);
void ht_insert(struct hashtable *ht, char* key, void* val);
void* ht_find(struct hashtable *ht, char* key);

int scan(struct context *ctx, FILE *fp);
int parse(struct context *ctx);
int out(struct context *ctx);
#endif
