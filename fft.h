#include <iostream>

using namespace std;

#ifndef MY_CPXV_TYPE
#define MY_CPXV_TYPE
typedef struct _cpxv_t {
	double re;
	double im;
} cpxv_t;
#endif

void DoFFT(const short xxx[], cpxv_t fdom[]);