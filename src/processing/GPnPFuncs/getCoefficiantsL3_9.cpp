
#include "processing/GPnP.h" 

namespace GPnP{
	double computeCoefficiantsL3_9(std::vector<std::vector<double> > k_coeff){
		double k11 = k_coeff[0][0];
		double k12 = k_coeff[0][1];
		double k13 = k_coeff[0][2];
		double k14 = k_coeff[0][3];
		double k15 = k_coeff[0][4];
		double k16 = k_coeff[0][5];

		double k21 = k_coeff[1][0];
		double k22 = k_coeff[1][1];
		double k23 = k_coeff[1][2];
		double k24 = k_coeff[1][3];
		double k25 = k_coeff[1][4];
		double k26 = k_coeff[1][5];

		double k31 = k_coeff[2][0];
		double k32 = k_coeff[2][1];
		double k33 = k_coeff[2][2];
		double k34 = k_coeff[2][3];
		double k35 = k_coeff[2][4];
		double k36 = k_coeff[2][5];

		double t0, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15, t16, t17;

#include "processing/GPnPFuncs/getCoefficiantsL3_9.h" 
		return t0;
	}
}