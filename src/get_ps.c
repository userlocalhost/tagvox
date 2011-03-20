#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_fft_complex.h>

#include "get_ps.h"

static void  fft (double *ar,int n,double flag);

#define PI 3.1415926535897932384626433832795
#define BUFLEN 512

#define REAL(z,i) ((z)[2*(i)])
#define IMAG(z,i) ((z)[2*(i)+1])

static void print_spectrum(double *, int);

static void swap(double a,double b) {
  double  w;
  w=a; a=b; b=w;
}

void get_spectrum(double *ar, int n)
{
	double *ap;
	int i;

	if(! ar){
		return;
	}

	/*
  fft(ar,n,0); // flag=0:FFT,-1:RFT

  for (ap=ar,i=0;i<n;i++,ap++) {
    *ap = sqrt(*ap * *ap)/n;
  }
	*/

	gsl_fft_complex_radix2_forward(ar, 1, n);

	for(i=0;i<n;i++){
		ar[i] = sqrt(REAL(ar,i)*REAL(ar,i) + IMAG(ar,i)*IMAG(ar,i))/n;
	}
}

// fftの計算
static void fft(double *ar,int n,double flg) {
  int     nh, nhlf;
  int     m0, m1;
  int     k0, k1;
  int     n0, n1;
  int     i,j;
  int     *mw;
  double  ark1;

  // work area for bit operation
  mw = (int *)malloc(n*sizeof(int));
  for(i=0;i<n;i++) mw[i] = 0;

  // FFT
  nh = nhlf = n/2;
  while(nh >= 1) {
    for(n0=0;n0<n;n0+=(2*nh)) {
      n1 = n0+nh;
      m0 = mw[n0]/2;
      m1 = m0 + nhlf;
      for(k0=n0;k0<n1;k0++) {
        k1 = k0+nh;
        ark1 = ar[k1];
        ar[k1] = ar[k0]-ark1;
        ar[k0] = ar[k0]+ark1;
        mw[k0] = m0;
        mw[k1] = m1;
      }
    }
    nh /= 2;
  }
  // responce for "flag"
  if(flg<0) { // flg<0: rft
    for(i=0;i<n;i++) ar[i] /= n;
  }
  // bit reverse operation
  for(i=0;i<n;i++) {
    j = mw[i];
    if( j>i ) swap(ar[i],ar[j]);
  }
  free(mw);
  return;
}
