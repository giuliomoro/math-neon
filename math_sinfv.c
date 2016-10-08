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

#ifdef NOT
float sinf_neon(float x)
{
	asm volatile (
	
	"vld1.32 		d3, [%0]				\n\t"	//d3 = {invrange, range}
	"vdup.f32 		d0, d0[0]				\n\t"	//d0 = {x, x}
	"vabs.f32 		d1, d0					\n\t"	//d1 = {ax, ax}
	
	"vmul.f32 		d2, d1, d3[0]			\n\t"	//d2 = d1 * d3[0] 
	"vcvt.u32.f32 	d2, d2					\n\t"	//d2 = (int) d2
	"vmov.i32	 	d5, #1					\n\t"	//d5 = 1	
	"vcvt.f32.u32 	d4, d2					\n\t"	//d4 = (float) d2	
	"vshr.u32 		d7, d2, #1				\n\t"	//d7 = d2 >> 1
	"vmls.f32 		d1, d4, d3[1]			\n\t"	//d1 = d1 - d4 * d3[1]
	
	"vand.i32 		d5, d2, d5			\n\t"	//d5 = d2 & d5
	"vclt.f32 		d18, d0, #0			\n\t"	//d18 = (d0 < 0.0)
	"vcvt.f32.u32 	d6, d5				\n\t"	//d6 = (float) d5
	"vmls.f32 		d1, d6, d3[1]		\n\t"	//d1 = d1 - d6 * d3[1]
	"veor.i32 		d5, d5, d7			\n\t"	//d5 = d5 ^ d7	
	"vmul.f32 		d2, d1, d1			\n\t"	//d2 = d1*d1 = {x^2, x^2}, xx = ax.f * ax.f
	
	"vld1.32 		{d16, d17}, [%1]	\n\t"	//q8 = {p7, p3, p5, p1}, __sinf_lut
	"veor.i32 		d5, d5, d18			\n\t"	//d5 = d5 ^ d18, n = n ^ m	
	"vshl.i32 		d5, d5, #31			\n\t"	//d5 = d5 << 31, n = n << 31
	"veor.i32 		d1, d1, d5			\n\t"	//d1 = d1 ^ d5, ax.i = ax.i ^ n
	
	"vmul.f32 		d3, d2, d2			\n\t"	//d3 = d2*d2 = {x^4, x^4}, xxxx = xx*xx
	"vmul.f32 		q0, q8, d1[0]		\n\t"	//q0 = q8 * d1[0] = {p7x, p3x, p5x, p1x}
	"vmla.f32 		d1, d0, d2[0]		\n\t"	//d1 = d1 + d0*d2 = {p5x + p7x^3, p1x + p3x^3}		
	"vmla.f32 		d1, d3, d1[0]		\n\t"	//d1 = d1 + d3*d0 = {...., p1x + p3x^3 + p5x^5 + p7x^7}		
	: 
	: "r"(__sinf_rng), "r"(__sinf_lut) 
    : "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7","q8", "q9"
	);
}
#endif
	//n = n ^ m;
	//m = (x < 0.0);
	//n = n ^ m;	
	//n = n << 31;
	//ax.i = ax.i ^ n; 
	//
	//xx = ax.f * ax.f;	
	//a = (__sinf_lut[0] * ax.f) * xx + (__sinf_lut[2] * ax.f);
	//b = (__sinf_lut[1] * ax.f) * xx + (__sinf_lut[3] * ax.f);
	//xx = xx * xx;
	//r = b + a * xx;
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
	asm volatile (
// assumes multiples of 4
	"loop:"
	"vld1.32 		d14, [%3]		\n\t"	//d14 = {invrange, range}
	"vld1.32 		{d0, d1}, [%0]!	\n\t"	//q0 = x[0:3]
	"vabs.f32 		q1, q0			\n\t"	//d1 = {ax, ax}

	"subs			%1, %1, 4       \n\t"
	"vmul.f32 		q2, q1, d14[0]\n\t"	//q2 = d1 * d14[0] 
	"vcvt.u32.f32 	q2, q2			\n\t"	//q2 = (int) q2
	"vmov.i32	 	q5, #1			\n\t"	//q5 = 1	
	"vcvt.f32.u32 	q4, q2			\n\t"	//q4 = (float) q2	
	"vshr.u32 		q8, q2, #1		\n\t"	//q8 = q2 >> 1
	"vmls.f32 		q1, q4, d14[1]	\n\t"	//q1 = q1 - q4 * d14[1]
	
	"vand.i32 		q5, q2, q5		\n\t"	//q5 = q2 & q5
	"vmov.i32		q15, q15   \n\t" // q15 = 0
	"vclt.f32 		q15, q0, q15	\n\t"	//q15 = (q0 < 0.0)
	"vcvt.f32.u32 	q6, q5			\n\t"	//q6 = (float) q5
	"vmls.f32 		q1, q6, d14[1]	\n\t"	//q1 = q1 - q6 * a23[1]
	"veor.i32 		q5, q5, q8		\n\t"	//q5 = q5 ^ d7	
	"vmul.f32 		q2, q1, q1		\n\t"	//q2 = q1*q1 = xx = ax * ax

	"vld1.32 		{d14, d15},[%4] \n\t"	//q7 = {p7, p3, p5, p1}, __sinf_lut
	"veor.i32 		q5, q5, q15		\n\t"	//q5 = q5 ^ q15, n = n ^ m
	"vshl.i32 		q5, q5, #31		\n\t"	//q5 = q5 << 31, n = n << 31
	"veor.i32 		q1, q1, q5		\n\t"	//q1 = q1 ^ d5, ax = ax ^ n
	
	"vmul.f32 		q3, q2, q2		\n\t"	//q3 = q2*q2  = xxxx
	//ax           q1
	//xx           q2
	//xxxx         q3
	//sinf_lut     q7

	//xxx          q0
	"vmul.f32 q0, q2, q1\n\t" //q0 = q1 * q2, xxx = xx * ax
	
	//a            q4
	//a = xxx * sinf_lut[0] + ax * sinf_lut[2]
	//in two steps: 
	"vmul.f32 q4, q0, d14[0]  \n\t" //q4 = q0 * q7[0], a = xxx * sinf_lut[0]
	"vmla.f32 q4, q1, d15[0]  \n\t" // q4 += q1 * q7[2], a += ax * sinf_lut[2]

	//b          q5
	//b = xxx * sinf_lut[1] + ax * sinf_lut[3]
	//in two steps:
	"vmul.f32 q5, q0, d14[1] \n\t"  //q5 = q0 * q7[1], b = xxx * sinf_lut[1] 
	"vmla.f32 q5, q1, d15[1] \n\t"  //q5 += q1 * q7[3], b += ax * sinf_lut[3] 

	"vmla.f32 q5, q4, q3 \n\t"     //r = b + a * xxxx;

	"vst1.32 		{d10, d11}, [%2]!	\n\t"	//d = q12	
	"it	gt				\n\t"
	"bgt loop \n\t"

	: "+r"(x), "+r"(n), "+r"(y)  // outputs 
	: "r"(__sinfv_rng), "r"(__sinfv_lut) // inputs
	: "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7" //clobber
	);
#else
	sinfv_c(x, n, r);
#endif
}
