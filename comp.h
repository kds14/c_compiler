#ifndef COMP_H
#define COMP_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

//#define PAR_DBG
//#define SCAN_DBG

enum token {TK_NON = 0x0, TK_TEXT = 0x1, TK_SEMICOL = 0x3B,
			TK_INT = 0x4, TK_OP = 0x40, TK_LPAREN = 0x28,
			TK_RPAREN = 0x29, TK_ASS = 0x3D, TK_LBRACE = 0x7B,
			TK_RBRACE = 0x7D, TK_ALL = 0xFFFFFFFF, TK_COMMA = 0x2C};

enum ast_type { AST_OP, AST_INT, AST_ASS, AST_VAR, AST_FUNC };

enum var_type { T_NON = 0, T_INT = 1, T_FUNC };
enum keyword { KW_NON = 0, KW_RET = 1 };

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

void token_str(struct token_node *t, char* str);

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
	union {
		struct {
			struct ast_node *left;
			struct ast_node *right;
		};
		struct {
			struct vector *many;
		};
	};
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

struct context {
	struct vector *tokens;
	struct vector *asts;
	struct hashtable *syms;
};

struct sym_ent {
	union {
		uint64_t addr;
		uint64_t stack;
	};

	// fuction fields
	enum var_type ret_type;
	struct hashtable *params;
	int defined;

	int on_stack;
	enum var_type type;
	char* name;
};

int scan(struct context *ctx, FILE *fp);
int parse(struct context *ctx);
int out(struct context *ctx);

#endif
