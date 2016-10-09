/*
The MIT License (MIT)

Copyright (c) 2015 Lachlan Tychsen-Smith (lachlan.ts@gmail.com)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "math.h"
#include "math_neon.h"

const float __sinfv_rng[2] = {
	2.0 / M_PI,
	M_PI / 2.0, 
};

const float __sinfv_lut[4] = {
	-0.00018365f,	//p7
	-0.16664831f,	//p3
	+0.00830636f,	//p5
	+0.99999661f,	//p1
};

void sinfv_c(float *x, int n, float *r)
{
	union {
		float 	f;
		int 	i;
	} ax, bx;
	
	float aa, ab, ba, bb, axx, bxx;
	int am, bm, an, bn;

	if (n & 0x1) {
		*r++ = sinf_c(*x++);
		n--;
	}

	float rng0 = __sinfv_rng[0];
	float rng1 = __sinfv_rng[1];

	while(n > 0){
		
		float x0 = *x++;
		float x1 = *x++;
		
		ax.f = fabsf(x0);
		bx.f = fabsf(x1);

		//Range Reduction:
		am = (int) (ax.f * rng0);	
		bm = (int) (bx.f * rng0);	
		
		ax.f = ax.f - (((float)am) * rng1);
		bx.f = bx.f - (((float)bm) * rng1);

		//Test Quadrant
		an = am & 1;
		bn = bm & 1;
		ax.f = ax.f - an * rng1;
		bx.f = bx.f - bn * rng1;
		am = (am & 2) >> 1;
		bm = (bm & 2) >> 1;
		ax.i = ax.i ^ ((an ^ am ^ (x0 < 0)) << 31);
		bx.i = bx.i ^ ((bn ^ bm ^ (x1 < 0)) << 31);
			
		//Taylor Polynomial (Estrins)
		axx = ax.f * ax.f;	
		bxx = bx.f * bx.f;	
		aa = (__sinfv_lut[0] * ax.f) * axx + (__sinfv_lut[2] * ax.f);
		ba = (__sinfv_lut[0] * bx.f) * bxx + (__sinfv_lut[2] * bx.f);
		ab = (__sinfv_lut[1] * ax.f) * axx + (__sinfv_lut[3] * ax.f);
		bb = (__sinfv_lut[1] * bx.f) * bxx + (__sinfv_lut[3] * bx.f);
		axx = axx * axx;
		bxx = bxx * bxx;
		*r++ = ab + aa * axx;
		*r++ = bb + ba * bxx;
		n -= 2;
	}
	
	
}

void sinfv_neon(float *x, int n, float *y)
{
#ifdef __MATH_NEON
	// take care of the % 4 values
	for(int k = 0; k < (n&3); ++k){
		*y++ = sinf_neon(*x++);
	}
	n ^= n&3;
	if(n == 0)
		return;
	asm volatile (
	"vld1.32 		{d12, d13},[%4] \n\t"	//q14 = {p7, p3, p5, p1}, __sinf_lut
	"vld1.32 		d14, [%3]		\n\t"	//d14 = {invrange, range}
// preload first input outside the loop
	"vld1.32 		{d0, d1}, [%0]!	\n\t"	//q0 = x[0:3]
// assumes multiples of 4
	"loop:"
	"vabs.f32 		q1, q0			\n\t"	//d1 = {ax, ax}

	"subs			%1, %1, 4       \n\t"
	"vmul.f32 		q2, q1, d14[0] \n\t"	//q2 = d1 * d14[0] 
	"vcvt.u32.f32 	q2, q2			\n\t"	//q2 = (int) q2
	"vmov.i32	 	q5, #1			\n\t"	//q5 = 1	
	"vcvt.f32.u32 	q4, q2			\n\t"	//q4 = (float) q2	
	"vshr.u32 		q8, q2, #1		\n\t"	//q8 = q2 >> 1
	"vmls.f32 		q1, q4, d14[1]	\n\t"	//q1 = q1 - q4 * d14[1]
	
	"vand.i32 		q5, q2, q5		\n\t"	//q5 = q2 & q5
	"vclt.f32 		q15, q0, #0		\n\t"	//q15 = (q0 < 0.0)
	"vcvt.f32.u32 	q14, q5			\n\t"	//q14 = (float) q5
	"veor.i32 		q5, q5, q8		\n\t"	//q5 = q5 ^ d7	
	"vmls.f32 		q1, q14, d14[1]	\n\t"	//q1 = q1 - q14 * a23[1]

	"veor.i32 		q5, q5, q15		\n\t"	//q5 = q5 ^ q15, n = n ^ m
	"vmul.f32 		q2, q1, q1		\n\t"	//q2 = q1*q1 = xx = ax * ax
	"vshl.i32 		q5, q5, #31		\n\t"	//q5 = q5 << 31, n = n << 31
	"veor.i32 		q1, q1, q5		\n\t"	//q1 = q1 ^ d5, ax = ax ^ n
	
	"vmul.f32 		q3, q2, q2		\n\t"	//q3 = q2*q2  = xxxx
	"vmul.f32 q0, q2, q1\n\t" //q0 = q1 * q2, xxx = xx * ax
	"vmul.f32 q4, q0, d12[0]  \n\t" //q4 = q0 * q6[0], a = xxx * sinf_lut[0]
	"vmul.f32 q5, q0, d12[1] \n\t"  //q5 = q0 * q6[1], b = xxx * sinf_lut[1] 

	"it le \n\t"
	"ble preloaded\n\t"
	// preload next input UNLESS we are at the end of the loop
	"vld1.32 		{d0, d1}, [%0]!	\n\t"	//q0 = x[0:3]
	"preloaded:	 \n\t"
	"vmla.f32 q4, q1, d13[0]  \n\t" // q4 += q1 * q6[2], a += ax * sinf_lut[2]
	"vmla.f32 q5, q1, d13[1] \n\t"  //q5 += q1 * q6[3], b += ax * sinf_lut[3] 

	"vmla.f32 q5, q4, q3 \n\t"     //r = b + a * xxxx;

	"vst1.32 		{d10, d11}, [%2]!	\n\t"	//d = q12	
	"it	gt				\n\t"
	"bgt loop \n\t"

	: "+r"(x), "+r"(n), "+r"(y)  // outputs 
	: "r"(__sinfv_rng), "r"(__sinfv_lut) // inputs
	: "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7", "q8", "q14", "q15", "cc" //clobber
	);
#else
	sinfv_c(x, n, r);
#endif
}
