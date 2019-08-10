#include "comp.h"

int main(int argc, char **argv) {
	FILE* fp = fopen(argv[1], "r");
	if (fp == NULL)
		return 0;
	struct context *ctx = calloc(1, sizeof(struct context));
	if (scan(ctx, fp) == -1)
		printf("SCAN ERROR\n");
	else if (parse(ctx) == -1)
		printf("PARSE ERROR\n");
	else if (out(ctx) == -1)
		printf("OUTPUT ERROR\n");
	return 0;
}
