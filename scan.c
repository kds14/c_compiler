#include "comp.h"

enum scan_err { INV_CH, PAREN_MISM, PAREN_OPEN};

struct scan_ctx {
	struct vector *buff;
	size_t size;
	struct token_node *next;
	struct vector *tokens;
	enum token scan_state;
	char last_char;
	uint64_t char_count;
	uint64_t parens;
	enum scan_err err_type;
};

void scan_err(struct scan_ctx* ctx) {
	switch (ctx->err_type) {
		case INV_CH:
			printf("Invalid char '%c' at %lu\n",
					ctx->last_char, ctx->char_count);
			break;
		case PAREN_MISM:
			printf("Paren mismatch '%c' at %lu\n",
					ctx->last_char, ctx->char_count);
			break;
		case PAREN_OPEN:
			printf("%lu unclosed parenthesis\n", ctx->parens);
			break;
	}
	exit(0);
}

void print_token(struct token_node *t) {
	char* str = calloc(1000, sizeof(char));
	token_str(t, str);
	printf("%s", str);
	free(str);
}

void token_str(struct token_node *t, char* str) {
	if (t->type == TK_INT) {
		sprintf(str, "%d", t->int_val);
	} else if (t->type == TK_TEXT) {
		sprintf(str, "%s", t->str_val);
	} else {
		sprintf(str, "%c", t->char_val);
	}
}

void print_tokens(struct vector *tokens) {
	vector_reset(tokens);
	struct token_node *t = NULL;
	int start = 1;
	while ((t = vector_next(tokens)) != NULL) {
		if (start) {
			start = 0;
		} else {
			printf(", ");
		}
		print_token(t);
	}
	puts("");
}

void commit_token(struct scan_ctx *scan_ctx) {
	int len = scan_ctx->buff->len;
	scan_ctx->buff->len = 0;
	char* val;
	switch (scan_ctx->scan_state) {
		case TK_INT:
			val = calloc(len, sizeof(char));
			memcpy(val, scan_ctx->buff->buff, len);
			scan_ctx->next->int_val = atoi(val);
			free(val);
			break;
		case TK_LPAREN:
		case TK_RPAREN:
		case TK_LBRACE:
		case TK_RBRACE:
		case TK_OP:
		case TK_SEMICOL:
		case TK_ASS:
			scan_ctx->next->char_val = ((char *)scan_ctx->buff->buff)[0];	
			break;
		case TK_TEXT:
			val = calloc(len + 1, sizeof(char));
			memcpy(val, scan_ctx->buff->buff, len);
			scan_ctx->next->str_val = val;
			scan_ctx->next->str_size = len;
			break;
		case TK_NON:
		default:
			return;
	}
	scan_ctx->next->type = scan_ctx->scan_state;
	vector_push_back(scan_ctx->tokens, scan_ctx->next);
	scan_ctx->next = calloc(1, sizeof(struct token_node));
}

void handle_single(struct scan_ctx* scan_ctx, char c) {
	if (scan_ctx->scan_state != TK_NON) {
		commit_token(scan_ctx);
	}
	if (c == '+' || c == '-' || c == '*' || c == '/') {
		scan_ctx->scan_state = TK_OP;
		vector_push_back(scan_ctx->buff, &c);
	} else if (c == ';' || c == '(' || c == ')' || c == '='
				|| c == ',' || c == '}' || c == '{') {
		scan_ctx->scan_state = (enum token)c;
		if (c == '(')
			scan_ctx->parens++;
		else if (c == ')' && scan_ctx->parens-- == 0) {
			scan_ctx->err_type = PAREN_MISM;
			scan_err(scan_ctx);
		}
		vector_push_back(scan_ctx->buff, &c);
	} else {
		scan_ctx->scan_state = TK_NON;
	}
}

int scan(struct context *ctx, FILE *fp) {
	char c;
	struct scan_ctx* scan_ctx = calloc(1, sizeof(struct scan_ctx));
	scan_ctx->next = calloc(1, sizeof(struct token_node));
	vector_init(&scan_ctx->tokens, sizeof(struct token_node), 100);
	vector_init(&scan_ctx->buff, sizeof(char), 100);
	scan_ctx->scan_state = TK_NON;

	while ((c = getc(fp)) != EOF) {
		scan_ctx->last_char = c;
		if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
			if (scan_ctx->scan_state != TK_TEXT) {
				commit_token(scan_ctx);
			}
			scan_ctx->scan_state = TK_TEXT;
			vector_push_back(scan_ctx->buff, &c);
		} else if (c >= '0' && c <= '9') {
			if (!(scan_ctx->scan_state & (TK_INT | TK_TEXT))) {
				commit_token(scan_ctx);
			}
			if (scan_ctx->scan_state != TK_TEXT) {
				scan_ctx->scan_state = TK_INT;
			}
			vector_push_back(scan_ctx->buff, &c);
		} else {
			handle_single(scan_ctx, c);
		} 
	}
	scan_ctx->char_count++;
	if (scan_ctx->scan_state != TK_NON) {
		commit_token(scan_ctx);
		free(scan_ctx->next);
	}
	if (scan_ctx->parens > 0) {
		scan_ctx->err_type = PAREN_OPEN;
		scan_err(scan_ctx);
	}
#ifdef SCAN_DBG
	print_tokens(scan_ctx->tokens);
#endif
	ctx->tokens = scan_ctx->tokens;
	return 0;
}


