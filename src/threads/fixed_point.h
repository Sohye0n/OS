#define FRACTION (1<<14)
#define F (1<<14)

#ifndef INTH
include ""
#endif

int i_mul_f(int i, int f);
int f_mul_f(int f1, int f2);
int f_div_f(int f1, int f2);
int i_div_f(int i, int f);

int i_add_f(int i, int f);
int f_add_f(int f1, int f2);
int f_sub_f(int f1, int f2);