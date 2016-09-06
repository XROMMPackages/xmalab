/* The computation of the butterworth low pass filter is taken from 
* http ://www.exstrom.com/journal/sigproc/liir.c with the following Copyright
*
*                            COPYRIGHT
*
*  liir - Recursive digital filter functions
*  Copyright (C) 2007 Exstrom Laboratories LLC
*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  A copy of the GNU General Public License is available on the internet at:
*
*  http://www.gnu.org/copyleft/gpl.html
*
*  or you can write to:
*
*  The Free Software Foundation, Inc.
*  675 Mass Ave
*  Cambridge, MA 02139, USA
*
*  You can contact Exstrom Laboratories LLC via Email at:
*
*  stefan(AT)exstrom.com
*
*  or you can write to:
*
*  Exstrom Laboratories LLC
*  P.O. Box 7651
*  Longmont, CO 80501, USA
*
*------------------------------------
* The filtfilt funtion from http://stackoverflow.com/questions/17675053/matlabs-filtfilt-algorithm/27270420#27270420
*
*/


//


#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "processing/ButterworthLowPassFilter.h" 
#ifndef M_PI
#define M_PI   3.14159265358979323846	
#endif

using namespace xma;

double* binomial_mult(int n, double* p)
{
	int i, j;
	double* a;

	a = (double *)calloc(2 * n, sizeof(double));
	if (a == NULL) return (NULL);

	for (i = 0; i < n; ++i)
	{
		for (j = i; j > 0; --j)
		{
			a[2 * j] += p[2 * i] * a[2 * (j - 1)] - p[2 * i + 1] * a[2 * (j - 1) + 1];
			a[2 * j + 1] += p[2 * i] * a[2 * (j - 1) + 1] + p[2 * i + 1] * a[2 * (j - 1)];
		}
		a[0] += p[2 * i];
		a[1] += p[2 * i + 1];
	}
	return (a);
}

double* dcof_bwlp(int n, double fcf)
{
	int k; // loop variables
	double theta; // M_PI * fcf / 2.0
	double st; // sine of theta
	double ct; // cosine of theta
	double parg; // pole angle
	double sparg; // sine of the pole angle
	double cparg; // cosine of the pole angle
	double a; // workspace variable
	double* rcof; // binomial coefficients
	double* dcof; // dk coefficients

	rcof = (double *)calloc(2 * n, sizeof(double));
	if (rcof == NULL) return (NULL);

	theta = M_PI * fcf;
	st = sin(theta);
	ct = cos(theta);

	for (k = 0; k < n; ++k)
	{
		parg = M_PI * (double)(2 * k + 1) / (double)(2 * n);
		sparg = sin(parg);
		cparg = cos(parg);
		a = 1.0 + st * sparg;
		rcof[2 * k] = -ct / a;
		rcof[2 * k + 1] = -st * cparg / a;
	}

	dcof = binomial_mult(n, rcof);
	free(rcof);

	dcof[1] = dcof[0];
	dcof[0] = 1.0;
	for (k = 3; k <= n; ++k)
		dcof[k] = dcof[2 * k - 2];
	return (dcof);
}

int* ccof_bwlp(int n)
{
	int* ccof;
	int m;
	int i;

	ccof = (int *)calloc(n + 1, sizeof(int));
	if (ccof == NULL) return (NULL);

	ccof[0] = 1;
	ccof[1] = n;
	m = n / 2;
	for (i = 2; i <= m; ++i)
	{
		ccof[i] = (n - i + 1) * ccof[i - 1] / i;
		ccof[n - i] = ccof[i];
	}
	ccof[n - 1] = n;
	ccof[n] = 1;

	return (ccof);
}

double sf_bwlp(int n, double fcf)
{
	int k; // loop variables
	double omega; // M_PI * fcf
	double fomega; // function of omega
	double parg0; // zeroth pole angle
	double sf; // scaling factor

	omega = M_PI * fcf;
	fomega = sin(omega);
	parg0 = M_PI / (double)(2 * n);

	sf = 1.0;
	for (k = 0; k < n / 2; ++k)
		sf *= 1.0 + fomega * sin((double)(2 * k + 1) * parg0);

	fomega = sin(omega / 2.0);

	if (n % 2) sf *= fomega + cos(omega / 2.0);
	sf = pow(fomega, n) / sf;

	return (sf);
}

typedef std::vector<int> vectori;
typedef std::vector<double> vectord;

void add_index_range(vectori& indices, int beg, int end, int inc = 1)
{
	for (int i = beg; i <= end; i += inc)
		indices.push_back(i);
}

void add_index_const(vectori& indices, int value, size_t numel)
{
	while (numel--)
		indices.push_back(value);
}

void append_vector(vectord& vec, const vectord& tail)
{
	vec.insert(vec.end(), tail.begin(), tail.end());
}

vectord subvector_reverse(const vectord& vec, int idx_end, int idx_start)
{
	vectord result(&vec[idx_start], &vec[idx_end + 1]);
	std::reverse(result.begin(), result.end());
	return result;
}

inline int max_val(const vectori& vec)
{
	return std::max_element(vec.begin(), vec.end())[0];
}


void filter(vectord B, vectord A, const vectord& X, vectord& Y, vectord& Zi)
{
	if (A.empty())
		throw std::domain_error("The feedback filter coefficients are empty.");
	if (std::all_of(A.begin(), A.end(), [](double coef)
	                {
		                return coef == 0;
	                }))
		throw std::domain_error("At least one of the feedback filter coefficients has to be non-zero.");
	if (A[0] == 0)
		throw std::domain_error("First feedback coefficient has to be non-zero.");

	// Normalize feedback coefficients if a[0] != 1;
	auto a0 = A[0];
	if (a0 != 1.0)
	{
		std::transform(A.begin(), A.end(), A.begin(), [a0](double v)
		               {
			               return v / a0;
		               });
		std::transform(B.begin(), B.end(), B.begin(), [a0](double v)
		               {
			               return v / a0;
		               });
	}

	size_t input_size = X.size();
	size_t filter_order = std::max(A.size(), B.size());
	B.resize(filter_order, 0);
	A.resize(filter_order, 0);
	Zi.resize(filter_order, 0);
	Y.resize(input_size);

	const double* x = &X[0];
	const double* b = &B[0];
	const double* a = &A[0];
	double* z = &Zi[0];
	double* y = &Y[0];

	for (size_t i = 0; i < input_size; ++i)
	{
		size_t order = filter_order - 1;
		while (order)
		{
			if (i >= order)
				z[order - 1] = b[order] * x[i - order] - a[order] * y[i - order] + z[order];
			--order;
		}
		y[i] = b[0] * x[i] + z[0];
	}
	Zi.resize(filter_order - 1);
}


void filtfilt(vectord B, vectord A, const vectord& X, vectord& Y)
{
	//using namespace Eigen;

	int len = X.size(); // length of input
	int na = A.size();
	int nb = B.size();
	int nfilt = (nb > na) ? nb : na;
	int nfact = 3 * (nfilt - 1); // length of edge transients

	if (len <= nfact)
		throw std::domain_error("Input data too short! Data must have length more than 3 times filter order.");

	// set up filter's initial conditions to remove DC offset problems at the
	// beginning and end of the sequence
	B.resize(nfilt, 0);
	A.resize(nfilt, 0);

	vectori rows, cols;
	//rows = [1:nfilt-1           2:nfilt-1             1:nfilt-2];
	add_index_range(rows, 0, nfilt - 2);
	if (nfilt > 2)
	{
		add_index_range(rows, 1, nfilt - 2);
		add_index_range(rows, 0, nfilt - 3);
	}
	//cols = [ones(1,nfilt-1)         2:nfilt-1          2:nfilt-1];
	add_index_const(cols, 0, nfilt - 1);
	if (nfilt > 2)
	{
		add_index_range(cols, 1, nfilt - 2);
		add_index_range(cols, 1, nfilt - 2);
	}
	// data = [1+a(2)         a(3:nfilt)        ones(1,nfilt-2)    -ones(1,nfilt-2)];

	auto klen = rows.size();
	vectord data;
	data.resize(klen);
	data[0] = 1 + A[1];
	int j = 1;
	if (nfilt > 2)
	{
		for (int i = 2; i < nfilt; i++)
			data[j++] = A[i];
		for (int i = 0; i < nfilt - 2; i++)
			data[j++] = 1.0;
		for (int i = 0; i < nfilt - 2; i++)
			data[j++] = -1.0;
	}

	vectord leftpad = subvector_reverse(X, nfact, 1);
	double _2x0 = 2 * X[0];
	std::transform(leftpad.begin(), leftpad.end(), leftpad.begin(), [_2x0](double val)
	               {
		               return _2x0 - val;
	               });

	vectord rightpad = subvector_reverse(X, len - 2, len - nfact - 1);
	double _2xl = 2 * X[len - 1];
	std::transform(rightpad.begin(), rightpad.end(), rightpad.begin(), [_2xl](double val)
	               {
		               return _2xl - val;
	               });

	double y0;
	vectord signal1, signal2, zi;

	signal1.reserve(leftpad.size() + X.size() + rightpad.size());
	append_vector(signal1, leftpad);
	append_vector(signal1, X);
	append_vector(signal1, rightpad);

	// Calculate initial conditions
	cv::Mat sp = cv::Mat::zeros(max_val(rows) + 1, max_val(cols) + 1,CV_64FC1);
	for (size_t k = 0; k < klen; ++k)
		sp.at<double>(rows[k], cols[k]) = data[k];

	cv::Mat bb = cv::Mat(B, true);
	cv::Mat aa = cv::Mat(A, true);
	cv::Mat zzi = (sp.inv() * (bb.rowRange(1, nfilt) - (aa.rowRange(1, nfilt) * bb.at<double>(0, 0))));
	zi.resize(zzi.rows);

	// Do the forward and backward filtering
	y0 = signal1[0];
	std::transform(zzi.begin<double>(), zzi.end<double>(), zi.begin(), [y0](double val)
	               {
		               return val * y0;
	               });
	filter(B, A, signal1, signal2, zi);
	std::reverse(signal2.begin(), signal2.end());
	y0 = signal2[0];
	std::transform(zzi.begin<double>(), zzi.end<double>(), zi.begin(), [y0](double val)
	               {
		               return val * y0;
	               });
	filter(B, A, signal2, signal1, zi);
	Y = subvector_reverse(signal1, signal1.size() - nfact - 1, nfact);
}

ButterworthLowPassFilter::ButterworthLowPassFilter(int order, double cutOffFrequency, double recordingFrequency)
{
	double fcf; // cutoff frequency (fraction of pi)

	n = order;
	fcf = cutOffFrequency / (recordingFrequency * 0.5);

	/* calculate the d coefficients */
	dcof = dcof_bwlp(n, fcf);

	/* calculate the c coefficients */
	ccof = ccof_bwlp(n);

	sf = sf_bwlp(n, fcf); /* scaling factor for the c coefficients */
}

ButterworthLowPassFilter::~ButterworthLowPassFilter()
{
	free(dcof);
	free(ccof);
}

void ButterworthLowPassFilter::filter(std::vector<double>& in, std::vector<double>& out)
{
	vectord B;
	vectord A;

	for (int i = 0; i <= n; ++i)
		B.push_back((double)ccof[i] * sf);

	for (int i = 0; i <= n; ++i)
		A.push_back((double)dcof[i]);

	filtfilt(B, A, in, out);
}

