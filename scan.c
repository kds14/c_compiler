#include "comp.h"

struct scan_ctx {
	struct vector *buff;
	size_t size;
	struct token_node *next;
	struct vector *tokens;
	enum token scan_state;
	uint16_t possible;
};

void print_tokens(struct vector *tokens) {
	vector_reset(tokens);
	struct token_node *t = NULL;
	while ((t = vector_next(tokens)) != NULL) {
		if (t->type == TK_INT) {
			printf("%d, ", t->int_val);
		} else if (t->type == TK_OP) {
			printf("%c, ", t->char_val);
		}
	}
}

void commit_token(struct scan_ctx *scan_ctx) {
	int len = scan_ctx->buff->len;
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
	ctx->tokens = scan_ctx->tokens;
	return 0;
}


