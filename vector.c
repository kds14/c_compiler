#include "comp.h"

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

void* vector_next(struct vector *v) {
	void* res = NULL;
	if ((char *)v->next >= (char *)v->buff + v->ele_size * v->len)
		return NULL;
	res = v->next;
	v->next = (char *)v->next + v->ele_size;
	return res;
}

void* vector_peek(struct vector *v) {
	if ((char *)v->next >= (char *)v->buff + v->ele_size * v->len)
		return NULL;
	return v->next;
}

void vector_reset(struct vector *v) {
	v->next = v->buff;
}

void* vector_back(struct vector *v) {
	return (char *)v->buff + v->len * v->ele_size;
}
