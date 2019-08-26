#include "comp.h"

/*
 * GRAMMAR USED
 * Expr ::= Term Expr'
 * Expr' ::= + Term Expr' | - Term Expr' | ϵ
 * Term ::= Factor Term'
 * Term' ::= * Factor Term' | / Factor Term' | ϵ
 * Factor ::= (Expr) | NUM | VAR | FUNC_CALL
 */


enum parse_exp {P_NON = 0x0, P_IDENT = 0x1};

enum parse_err {PE_NON = 0, PE_DUPE_VAR, PE_CONS, PE_VARB4ASS,
				PE_SYMDNE, PE_NOTFUNC};

struct parse_ctx {
	struct vector *tokens;
	char **types;
	size_t types_size;
	char **kws;
	size_t kws_size;
	struct hashtable *syms;
	enum parse_err err;
	char* err_ex;
	int braces;
	int paren;
};

struct ast_node *expr(struct parse_ctx *ctx);
enum var_type get_type(struct parse_ctx* ctx, struct token_node *node);
struct ast_node *line(struct parse_ctx *ctx);
struct ast_node *func_call(struct parse_ctx *ctx, char* ident);

int get_ast_height(struct ast_node *ast) {
	if (ast == NULL)
		return -1;
	int lh = get_ast_height(ast->left);
	int rh = get_ast_height(ast->right);
	return lh > rh ? lh + 1 : rh + 1;
}

void print_ast(struct ast_node *ast, int indent) {
	if (ast == NULL)
		return;
	print_ast(ast->right, indent + 8);
	for (int i = 0; i < indent; ++i)
		printf(" ");
	if (ast->type == AST_INT)
		printf("%d\n", ast->int_val);
	else if (ast->type == AST_OP || ast->type == AST_ASS)
		printf("%c\n", ast->char_val);
	else if (ast->type == AST_VAR)
		printf("var:%s\n", ast->ident);
	print_ast(ast->left, indent + 8);
}

void err_abort(struct parse_ctx *ctx) {
	struct token_node* node = vector_peek(ctx->tokens);
	char* str = calloc(1000, sizeof(char));
	token_str(node, str);
	printf("Parse error next token: %s", str);
	free(str);
	switch (ctx->err) {
		case PE_DUPE_VAR:
			printf(" (Duplicate assignment) ");
			break;
		case PE_CONS:
			printf(" (Consume error) ");
			break;
		case PE_VARB4ASS:
			printf(" (Variable %s used before assignment) ", ctx->err_ex);
			break;
		case PE_SYMDNE:
			printf(" (Symbol %s not in scope) ", ctx->err_ex);
			break;
		case PE_NOTFUNC:
			printf(" (Symbol %s not a function) ", ctx->err_ex);
			break;
		default:
			printf(" (Default) ");
			break;
	}
	printf("LINE: %lu\n", node->line);
	exit(0);
}

struct sym_ent *get_sym(struct parse_ctx *ctx, char* ident) {
	struct sym_ent *res;
	if ((res = ht_find(ctx->syms, ident)) == NULL) {
		ctx->err = PE_SYMDNE;
		ctx->err_ex = ident;
		err_abort(ctx);
	}
	return res;
}

struct ast_node *make_ast_var_node(enum var_type type, char* ident) {
	struct ast_node* res = calloc(1, sizeof(struct ast_node));
	res->vtype = type;
	res->ident = ident;
	res->type = AST_VAR;
	res->left = NULL;
	res->right = NULL;
	return res;
}

struct ast_node *make_ast_node(struct token_node* node,
		struct ast_node *left, struct ast_node *right) {
	struct ast_node* res = calloc(1, sizeof(struct ast_node));
	if (node->type == TK_INT) {
		res->type = AST_INT;
		res->int_val = node->int_val;
	} else if (node->type == TK_OP) {
		res->type = AST_OP;
		res->char_val = node->char_val;
	} else if (node->type == TK_ASS) {
		res->type = AST_ASS;
		res->char_val = node->char_val;
	}
	res->left = left;
	res->right = right;
	return res;
}

int consume(struct parse_ctx *ctx, uint64_t val) {
	struct token_node* node = vector_peek(ctx->tokens);
	if (node == NULL)
		err_abort(ctx);
#ifdef PAR_DBG
	char *str = calloc(1024, sizeof(char));
	token_str(node, str);
	printf("CONSUME %s\n", str);
	free(str);
#endif
	if (node->type & val) {
		vector_next(ctx->tokens);
		return 0;
	}
	ctx->err = PE_CONS;
	err_abort(ctx);
	return 1;
}


struct ast_node *factor(struct parse_ctx *ctx) {
	struct token_node *next, *node = vector_peek(ctx->tokens);
	struct ast_node *res = NULL;
	struct sym_ent *se;
	
	if (node->type == TK_LPAREN) {
		consume(ctx, TK_LPAREN);
		res = expr(ctx);
		consume(ctx, TK_RPAREN);
	} else if (node->type == TK_TEXT) {
		consume(ctx, TK_TEXT);
		char* name = node->str_val;
		if ((se = ht_find(ctx->syms, name)) == NULL) {
			ctx->err = PE_VARB4ASS;
			ctx->err_ex = name;
			err_abort(ctx);
		} else {
			next = vector_peek(ctx->tokens);
			if (next->type == TK_LPAREN) {
				consume(ctx, TK_LPAREN);
				return func_call(ctx, se->name);
			} else {
				return make_ast_var_node(se->type, se->name);
			}
		}
	} else if (node->type == TK_INT) {
		consume(ctx, TK_INT);
		return make_ast_node(node, NULL, NULL);
	}
	return res;
};

struct ast_node *term_prime(struct parse_ctx *ctx,
		struct ast_node *left) {
	struct ast_node *f, *tp, *res = left;
	struct token_node *next = vector_peek(ctx->tokens);
	while (next && next->type == TK_OP
			&& (next->char_val == '*' || next->char_val == '/')
			&& !consume(ctx, TK_OP)) {
		f = factor(ctx);
		if (f == NULL)
			err_abort(ctx);
		res = make_ast_node(next, res, f);
		next = vector_peek(ctx->tokens);
	}
	return res;
}

struct ast_node *term(struct parse_ctx *ctx) {
	struct ast_node *f = factor(ctx);
	if (f == NULL)
		err_abort(ctx);
	struct ast_node *tp = term_prime(ctx, f);
	if (tp == NULL)
		return f;
	return tp;
}

struct ast_node *expr_prime(struct parse_ctx *ctx,
		struct ast_node *left) {
	struct ast_node *t, *ep, *res = left;
	struct token_node *next = vector_peek(ctx->tokens);
	while (next && next->type == TK_OP && (next->char_val == '+'
				|| next->char_val == '-') && !consume(ctx, TK_OP))
	{
		t = term(ctx);
		res = make_ast_node(next, res, t);
		next = vector_peek(ctx->tokens);
	}
	return res;
}

struct ast_node *expr(struct parse_ctx *ctx) {
	struct ast_node *f = term(ctx);
	if (f == NULL)
		err_abort(ctx);
	struct ast_node *ep = expr_prime(ctx, f);
	if (ep == NULL)
		return f;
	return ep;
}

enum var_type get_type(struct parse_ctx* ctx, struct token_node *node) {
	if (node->type != TK_TEXT)
		return 0;
	for
(size_t i = 0;
		i < ctx->types_size;
		++i) {
		if (!strcmp(ctx->types[i], node->str_val)) {
			return i + 1;
		}
	}
	return 0;
}

enum keyword get_kw(struct parse_ctx* ctx, struct token_node *node) {
	if (node->type != TK_TEXT)
		return 0;
	for (int i = 0; i < ctx->kws_size; ++i) {
		if (!strcmp(ctx->kws[i], node->str_val)) {
			return i + 1;
		}
	}
	return 0;
}

void func_params(struct parse_ctx *ctx, struct hashtable *params) {
	struct token_node *next, *next2;
	next = vector_peek(ctx->tokens);
	enum var_type type;
	while (next->type != TK_RPAREN) {
		consume(ctx, TK_TEXT);
		type = get_type(ctx, next);
		if (!type)
			err_abort(ctx);
		next2 = vector_peek(ctx->tokens);
		consume(ctx, TK_TEXT);
		consume(ctx, TK_COMMA);
		struct sym_ent *se = calloc(1, sizeof(struct sym_ent));
		se->type = type;
		se->name = next2->str_val;
		ht_insert(params, se->name, se);
		next = vector_peek(ctx->tokens);
	}
	consume(ctx, TK_RPAREN);
}

struct ast_node *func_call(struct parse_ctx *ctx, char* ident) {
	struct ast_node *res;
	struct sym_ent *se = get_sym(ctx, ident);
	if (!se->func) {
		ctx->err = PE_NOTFUNC;
		ctx->err_ex = ident;
		err_abort(ctx);
	}
	struct token_node *next = vector_peek(ctx->tokens);
	res = make_ast_var_node(se->ret_type, ident);
	res->type = AST_CALL;
	vector_init(&res->many, sizeof(struct ast_node), 100);
	while (next->type != TK_RPAREN) {
		vector_push_back(res->many, expr(ctx));
		next = vector_peek(ctx->tokens);
		if (next->type == TK_COMMA)
			consume(ctx, TK_COMMA);
		next = vector_peek(ctx->tokens);
	}
	consume(ctx, TK_RPAREN);
	return res;
}

struct ast_node *func(struct parse_ctx *ctx, enum var_type ret_type,
		char* ident) {
	struct ast_node *res;
	struct token_node *next = vector_peek(ctx->tokens);
	res = make_ast_var_node(ret_type, ident);
	res->type = AST_FUNC;
	vector_init(&res->many, sizeof(struct ast_node), 100);
	ctx->braces = 1;
	while (next->type != TK_RBRACE || ctx->braces > 1) {
		vector_push_back(res->many, line(ctx));
		next = vector_peek(ctx->tokens);
	}
	return res;
}

struct ast_node *line(struct parse_ctx *ctx) {
	struct token_node *next, *next2, *next3;
	struct ast_node *res, *var;
	next = vector_peek(ctx->tokens);
	res = NULL;
	int semicol = 1;
	if (next == NULL)
		return NULL;
	enum var_type type = get_type(ctx, next);
	enum keyword kw = get_kw(ctx, next);
	if (type) {
		consume(ctx, TK_TEXT);
		next2 = vector_peek(ctx->tokens);
		consume(ctx, TK_TEXT);
		next3 = vector_peek(ctx->tokens);
		if (next3->type == TK_ASS) {
			consume(ctx, TK_ASS);
			char* name = next2->str_val;
			if (ht_find(ctx->syms, name)) {
				ctx->err = PE_DUPE_VAR;
				err_abort(ctx);
			}
			var = make_ast_var_node(type, name);
			struct sym_ent *se = calloc(1, sizeof(struct sym_ent));
			se->type = type;
			se->name = name;
			ht_insert(ctx->syms, name, se);
			res = make_ast_node(next3, var, expr(ctx));
		} else if (next3->type == TK_LPAREN) {
			consume(ctx, TK_LPAREN);
			char* name = next2->str_val;
			var = make_ast_var_node(type, name);
			struct sym_ent *se = calloc(1, sizeof(struct sym_ent));
			se->ret_type = type;
			se->type = type;
			se->name = name;
			se->func = 1;
			ht_init(&se->params, 10, 0.75f);
			func_params(ctx, se->params);
			ht_insert(ctx->syms, name, se);
			next3 = vector_peek(ctx->tokens);
			struct sym_ent *node = ht_find(ctx->syms, name);
			if (next3->type == TK_LBRACE) {
				semicol = 0;
				if (node != NULL && node->defined) {
					ctx->err = PE_DUPE_VAR;
					err_abort(ctx);
				}
				consume(ctx, TK_LBRACE);
				res = func(ctx, type, name);
				consume(ctx, TK_RBRACE);
				ctx->braces = 0;
			} else if (node != NULL) {
				ctx->err = PE_DUPE_VAR;
				err_abort(ctx);
			}
		}
	} else if (kw) {
		if (kw == KW_RET) {
			consume(ctx, TK_TEXT);
		}
		res = expr(ctx);
	}
	if (semicol)
		consume(ctx, TK_SEMICOL);
	return res;
}

int parse(struct context *ctx) {
	vector_reset(ctx->tokens);
	struct token_node *node = NULL;
	struct parse_ctx *parse_ctx = calloc(1, sizeof(struct parse_ctx));
	parse_ctx->types_size = 1;
	parse_ctx->types = calloc(parse_ctx->types_size, sizeof(char *));
	parse_ctx->types[0] = "int";
	parse_ctx->kws_size = 1;
	parse_ctx->kws = calloc(parse_ctx->types_size, sizeof(char *));
	parse_ctx->kws[0] = "return";

	ht_init(&parse_ctx->syms, 100, 0.75f);
	vector_init(&ctx->asts, sizeof(struct ast_node), 100);
	parse_ctx->tokens = ctx->tokens;
	struct ast_node *ptr;
	do {
		ptr = line(parse_ctx);
		if (ptr != NULL)
			vector_push_back(ctx->asts, ptr);
	} while (ptr != NULL);
	ctx->syms = parse_ctx->syms;
	return 0;
}
