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

struct context {
	struct token_node* tokens;
};

struct vector {
	void* buff;
	size_t len;
	size_t cap;
	size_t ele_size;
	void* next;
};


struct scan_ctx {
	struct vector *buff;
	size_t size;
	struct token_node *next;
	struct vector *tokens;
	enum token scan_state;
	uint16_t possible;
};

void vector_init(struct vector **vec, size_t ele_size, size_t cap) {
	*vec = calloc(1, sizeof(struct vector));
	struct vector *v = *vec;
	v->buff = malloc(cap * ele_size);
	v->ele_size = ele_size;
	v->cap = cap;
	v->len = 0;
	v->next = v->buff;
}

void vector_destroy(struct vector *v) {
	free(v->buff);
	free(v);
}

void vector_push_back(struct vector *v, void* val) {
	if (v->len == v->cap) {
		char* tmp = malloc(v->cap * v->ele_size);
		memcpy(tmp, v->buff, v->cap * v->ele_size);
		free(v->buff);
		v->cap *= 2;
		v->buff = calloc(v->cap, v->ele_size);
		memcpy(v->buff, tmp, v->len * v->ele_size);
		free(tmp);
	}
	void* end = (char *)v->buff + v->len * v->ele_size;
	memcpy(end, val, v->ele_size);
	v->len += 1;
}

/*
 * Iterate through vector, if reset == 1 will start at beginning
 */
void* vector_next(struct vector *v, int reset) {
	void* res = NULL;
	if (reset) {
		v->next = v->buff;
	}
	if ((char *)v->next >= (char *)v->buff + v->ele_size * v->len)
		return NULL;
	res = v->next;
	v->next = (char *)v->next + v->ele_size;
	return res;
}

void print_tokens(struct vector *tokens) {
	tokens->next = tokens->buff;
	struct token_node *t = NULL;
	while ((t = vector_next(tokens, 0)) != NULL) {
		if (t->type == TK_INT) {
			printf("%d, ", t->int_val);
		} else if (t->type == TK_OP) {
			printf("%c, ", t->char_val);
		}
	}
}

void commit_token(struct scan_ctx *scan_ctx) {
	int len = scan_ctx->buff->len;
	printf("LEN: %d\n", len);
	scan_ctx->buff->len = 0;
	if (scan_ctx->scan_state == TK_NON) {
		return;
	} else if (scan_ctx->scan_state == TK_INT) {
		char* val = calloc(1, len);
		memcpy(val, scan_ctx->buff->buff, len);
		scan_ctx->next->int_val = atoi(val);	
		scan_ctx->next->type = TK_INT;
		free(val);
	} else if (scan_ctx->scan_state == TK_OP) {
		scan_ctx->next->char_val = ((char *)scan_ctx->buff->buff)[0];	
		scan_ctx->next->type = TK_OP;
	}
	vector_push_back(scan_ctx->tokens, scan_ctx->next);
	scan_ctx->next = calloc(1, sizeof(struct token_node));
}

int scan(struct context *ctx, FILE *fp) {
	char c;
	struct scan_ctx* scan_ctx = calloc(1, sizeof(struct scan_ctx));
	scan_ctx->next = calloc(1, sizeof(struct token_node));
	vector_init(&scan_ctx->tokens, sizeof(struct token_node), 100);
	vector_init(&scan_ctx->buff, sizeof(char), 100);
	scan_ctx->possible = 0xFF;
	scan_ctx->scan_state = TK_NON;

	while ((c = getc(fp)) != EOF) {
		if (c >= '0' && c <= '9') {
			printf("NUM %c\n", c);
			if (!(scan_ctx->possible & TK_INT))
				return -1;
			if (scan_ctx->scan_state != TK_INT) {
				commit_token(scan_ctx);
			}
			scan_ctx->possible = TK_INT | TK_OP;
			scan_ctx->scan_state = TK_INT;
			vector_push_back(scan_ctx->buff, &c);
		} else if (c == '+' || c == '-' || c == '*' || c == '/') {
			if (!(scan_ctx->possible & TK_OP))
				return -1;
			if (scan_ctx->scan_state != TK_NON) {
				commit_token(scan_ctx);
			}
			scan_ctx->possible = TK_INT;
			scan_ctx->scan_state = TK_OP;
			vector_push_back(scan_ctx->buff, &c);
		} else {
			if (scan_ctx->scan_state != TK_NON) {
				commit_token(scan_ctx);
			}
			scan_ctx->scan_state = TK_NON;
			scan_ctx->possible = 0xFF;
		}
	}
	if (scan_ctx->scan_state != TK_NON) {
		commit_token(scan_ctx);
		free(scan_ctx->next);
	}
	print_tokens(scan_ctx->tokens);
	return 0;
}

int main(int argc, char **argv) {
	FILE* fp = fopen(argv[1], "r");
	if (fp == NULL)
		return 0;
	struct context *ctx = calloc(1, sizeof(struct context));
	if (scan(ctx, fp) == -1)
		printf("ERROR\n");
	return 0;
}
