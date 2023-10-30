#include "fixed_point.h"

int i_mul_f(int i, int f){
    return i*f;
}
int f_mul_f(int f1, int f2){
    long ret=f1*f2/F;
    return (int)ret;
}
int f_div_f(int f1, int f2){
    long ret=(f1/f2)*F;
    return (int)ret;
}
int i_div_f(int i, int f){
    return f/i;
}

int i_add_f(int i, int f){
    return i*F+f;
}
int f_add_f(int f1, int f2){
    return f1+f2;
}
int f_sub_f(int f1, int f2){
    return f1-f2;
}