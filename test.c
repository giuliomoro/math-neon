#include <stdio.h>
#include <math_neon.h>
#include <math.h>
#define USE_XENOMAI

#ifdef USE_XENOMAI
#include <native/task.h>
#include <native/timer.h>
#endif /* USE_XENOMAI */

#define numFuncs 40*2
float (*funcs[numFuncs])(float) = {
	sinf, sinf_neon,
	cosf, cosf_neon,		
	//sincosf, sincosf_neon,
	tanf, tanf_neon,
	atanf, atanf_neon,
	//atan2f, atan2f_neon,
	asinf, asinf_neon,
	acosf, acosf_neon,
	sinhf, sinhf_neon,
	coshf, coshf_neon,
	tanhf, tanhf_neon,
	expf, expf_neon,
	logf, logf_neon,
	log10f, log10f_neon,
	//powf, powf_neon,
	floorf, floorf_neon,
	ceilf, ceilf_neon,
	fabsf, fabsf_neon,
	//ldexpf, ldexpf_neon,
	//frexpf, frexpf_neon,
	//fmodf, fmodf_neon,
	//modf, modf_neon,
	sqrtf, sqrtf_neon
	//invsqrtf, invsqrtf_neon,
};

char* funcNames[numFuncs]={
	"sinf",
	"cosf",
	//"sincosf",
	"tanf",
	"atanf",
	//"atan2f",
	"asinf",
	"acosf",
	"sinhf",
	"coshf",
	"tanhf",
	"expf",
	"logf",
	"log10f",
	//"powf",
	"floorf",
	"ceilf",
	"fabsf",
	//"ldexpf",
	//"frexpf",
	//"fmodf",
	//"modf",
	"sqrtf",
	//"invsqrtf",
};

float max(float first, float second){
	return first > second ? first : second;
}
//maximum absolute, maximum relative and root mean squared 
float maxAbsErr(float** y, unsigned int length){
	float maxAbsErr = 0;
	for(unsigned int n = 0; n < length; ++n){
		float absErr = fabsf(y[0][n] - y[1][n]);
		maxAbsErr = max(absErr, maxAbsErr);
	}
	return maxAbsErr;
}

float maxRelErr(float** y, unsigned int length){
	float maxRelErr = 0;
	for(unsigned int n = 0; n < length; ++n){
		float ref = y[0][n] != 0 ? y[0][n] : y[1][n];
		float relErr;
		if(ref == 0){
			relErr = 0;
		} else {
			relErr = fabsf(y[0][n] - y[1][n]) / ref;
		}
		maxRelErr = max(relErr, maxRelErr);
	}
	return maxRelErr;
}

float rmsErr(float** y, unsigned int length){
	float sum = 0;
	for(unsigned int n = 0; n < length; ++n){
		float err = (y[0][n] - y[1][n]);
		sum += err * err;
	}
	return sqrt(sum / length);
}

#define numMetrics 3
float (*metrics[numMetrics])(float**, unsigned int) = {
	maxAbsErr,
	maxRelErr,
	rmsErr,
};

int main(){

	float a[10] = {1, 1.5, 2, 2.5, 3, 3.5, 4, 4.5, 5, 5.5};
	float out_c[10];
	float out_neon[10];
	sqrtfv_c(a, 10, out_c);
	sqrtfv_neon(a, 10, out_neon);
	if(1)
	for(int n = 0; n < 10; ++n){
		printf("%12.7f\n", out_c[n]);
		printf("%12.7f\n", out_neon[n]);
		printf("%12.7f\n", sqrtf_c(a[n]));
		printf("%12.7f\n", sqrtf_neon(a[n]));
		printf("%12.7f\n", sqrtf(a[n]));
		printf("\n");
	}


#ifdef USE_XENOMAI
	rt_task_shadow(NULL, "math-neon test", 95, T_FPU);
#endif /* USE_XENOMAI */
	//enable_runfast();	
	const unsigned int testLength = 500001;
	float x[testLength];
	float y1[testLength];
	float y2[testLength];
	float* y[2] = {y1, y2};
	for(unsigned int n = 0; n < testLength; ++n){
		//x[n] = -M_PI + n / (float)testLength * (2 * M_PI - 0.0001);
		x[n] = 1 + n / (float)testLength * 1000;
	}
	int numTrials = 10;
	enable_runfast();

	RTIME time;
	time = rt_timer_read();
	for(int n = 0; n < numTrials; ++n){
		//sinfv_c(x, testLength, y1);
		sqrtfv_c(x, testLength, y1);
	}
	printf("Elapsed c: %.4f\n", ((rt_timer_read() - time)/100000) / 10000.f);

	rt_task_sleep(1000000);
	time = rt_timer_read();
	for(int n = 0; n < numTrials; ++n){
		//sinfv_neon(x, testLength, y2);
		sqrtfv_neon(x, testLength, y2);
	}
	printf("Elapsed neon: %.4f\n", ((rt_timer_read() - time)/100000) / 10000.f);
	rt_task_sleep(1000000);
	
	int failed = 0;
	for(unsigned int n = 0; n < testLength; ++n){
		float lib = y1[n];
		float neon = y2[n];
		float precise = sqrtf(x[n]);
		float err = fabsf(precise - neon);
		float err2 = fabsf(precise - lib);
		if( err > 0.00001 || err2 > 0.01){
			printf("[%d]%.5f      c:%12.7f nv:%12.7f n:%12.7f err:%0.4f\n", n, precise, lib, neon, sqrtf(x[n]), err2);
			++failed;
			if(failed > 10){
				printf("Too many errors, stopping\n");
				exit(1);
			}
		}
	}
	printf("%s\n", failed ? "FAILED" : "Success");



	return 0;
	printf( "     func   maxAbs      maxRel      rms      t-c   t-neon\n");
	for(unsigned int n = 0; n < numFuncs; n += 2){
		float (*math[2])(float);
		int skip = 0;
		RTIME elapsed[2];
		for(unsigned int j = 0; j < 2; ++j){
			math[n] = funcs[n + j];
			if(math[n] == NULL){
				skip = 1;
				break;
			}
			RTIME time = rt_timer_read();
			for(unsigned int k = 0; k < testLength; ++k){
				y[j][k] = math[n](x[k]);
			}
			elapsed[j] = rt_timer_read() - time;
#ifdef USE_XENOMAI
			rt_task_sleep(1000000);//just to avoid SIGXCPU
#endif /* USE_XENOMAI */
		}
		if(skip == 1)
			break;
		printf("%10s ", funcNames[n/2]);
		for(unsigned int j = 0; j < numMetrics; ++j){
			printf(" %.8f",  metrics[j](y, testLength));
		}
		for(unsigned int j = 0; j < 2; ++j){
			printf(" %.3fs", (elapsed[j] / 1e6) / (float)1000);
		}
		printf("\n");
	}
}

