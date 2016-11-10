// GNU General Public License Agreement
// Copyright (C) 2004-2010 CodeCogs, Zyba Ltd, Broadwood, Holford, TA5 1DU, England.
//
// This program is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by CodeCogs. 
// You must retain a copy of this licence in all copies. 
//
// This program is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
// PARTICULAR PURPOSE. See the GNU General Public License for more details.
// ---------------------------------------------------------------------------------
//! Interpolates a given set of points using cubic spline fitting.
// Code was converted to C++ from "Cubic spline interpolation - a review" by George Wolberg
// https://academiccommons.columbia.edu/download/fedora_content/download/ac:142812/CONTENT/cucs-389-88.pdf
 

#ifndef MATHS_INTERPOLATION_CUBIC_H
#define MATHS_INTERPOLATION_CUBIC_H

#include <assert.h>
#include <vector>

namespace Maths
{

	namespace Interpolation
	{

		/*******************************************************************
		Gauss Elimination with backsubstitution for general
		tridiagonal matrix with bands A,B,C and column vector O.
		******************************************************************/
		bool tridiag(std::vector<double> &A, std::vector<double> &B, std::vector<double> &C, std::vector<double> &D)
		{
			int i;
			double b;
			std::vector<double> F;
			F.push_back(0);

			b = B[0];
			D[0] = D[0] / b;

			for (i = 1 ; i < A.size(); i++) {
				F.push_back(C[i - 1] / b);
				b = B[i] - A[i] * F[i];
				if (b == 0)
					//divide by 0 abort;
					return false;
				
				D[i] = (D[i] - D[i - 1] * A[i]) / b;
			}

			//backsubstituiton
			for (i = A.size() - 2; i >= 0;  i--)
				D[i] -= (D[i + 1] * F[i + 1]);
		}

		/*******************************************************************
		YO <-Computed 1st derivative of data in X, Y(len entries)
		The not - a - knot boundary condition is used
		******************************************************************/
		bool getYD(std::vector<double> X, std::vector<double> Y, std::vector<double> & YD)
		{
			int i;
			double h0, h1, r0, r1;
			std::vector<double> A, B, C;
			//init first row data

			h0 = X[1] - X[0];
			h1 = X[2] - X[1];
			r0 = (Y[1] - Y[0]) / h0;
			r1 = (Y[2] - Y[1]) / h1;

			A.push_back(0);
			B.push_back(h1 * (h0+h1));
			C.push_back((h0 + h1) * (h0 + h1));
			YD.push_back(r0 * (3 * h0*h1 + 2 * h1*h1) + r1*h0*h0);
			//init tridiagonal bands A, B, C, and column vector Y0 
			//Y0 will later be used to return the derivatives
			for (i = 1; i < X.size() - 1; i++) {
				h0 = X[i] - X[i - 1];
				h1 = X[i + 1] - X[i];
				r0 = (Y[i] - Y[i - 1]) / h0;
				r1 = (Y[i + 1] - Y[i]) / h1;
				A.push_back(h1);
				B.push_back(2 * (h0 + h1));
				C.push_back(h0);
				YD.push_back(3 * (r0*h1 + r1*h0));
			}

			//last row
			A.push_back((h0 + h1) * (h0+h1));
			B.push_back(h0 * (h0+h1));
			YD.push_back(r0*h1*h1 + r1*(3 * h0*h1 + 2 * h0*h0));
			//solve for the tridiagonal matrix : YO = YO'inv(tridiag matrix)
			return tridiag(A, B, C, YD);		
		}


		/*******************************************************************
		Interpolating cubic spline function for irregularly - spaced points
		Input : Y1 is a list of irregular data points(len1 entries)
		Their x - coordinates are spec~ied in X1
		Output : Y2 <-cubic spline sampled according to X2(len2 entries)
		Assume that X1, X2 entries are monotonically increasing
		******************************************************************/
		bool ispline(std::vector<double> X1, std::vector<double> Y1, std::vector<double> X2, std::vector<double> &Y2)
		{
			int i, j;
			std::vector<double> YD;
			Y2.clear();
			double	AO, A1, A2, A3, x, dx, dy, p1, p2, p3;
			
			//compute 1 st derivatives at each point->YD
			if (!getYD(X1, Y1, YD))
				return false;
			
			//error checking
			if (X2[0] < X1[0] || X2[X2.size() - 1] > X1[X1.size() - 1])
				return false;
			
			//p1 is left endpoint of interval
			//p2 is resampling position
			//p3 is right endpoint of interval
			//j is input index for current interval

			p3 = X2[0] - 1; //r force coefficient initialization
			for (i = j = 0; i < X2.size(); i++) {
				//check if in new interval 
				p2 = X2[i];
				if (p2 > p3){
					//find the interval which contains p2
					for (; j<X1.size() && p2>X1[j]; j++);

					if (p2 < X1[j]) j--;
					p1 = X1[j];			//update left endpoint
					p3 = X1[j + 1];		//update right endpoint

					//compute spline coefficients
					dx = 1.0 / (X1[j + 1] - X1[j]);
					dy = (Y1[j + 1] - Y1[j]) * dx;
					AO = Y1[j];
					A1 = YD[j];
					A2 = dx * (3.0*dy - 2.0*YD[j] - YD[j + 1]);
					A3 = dx * dx * (-2.0*dy + YD[j] + YD[j + 1]);
				}
				//use Horner's rule to calculate cubic polynomial
				x = p2 - p1;
				Y2.push_back(((A3*x + A2)*x + A1)*x + AO);
			}
			return true;
		}
	}
}

#endif