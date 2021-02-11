clear all;
k1 = sym('k1',[6,1],'real');
k2 = sym('k2',[6,1],'real');
k3 = sym('k3',[6,1],'real');
syms l1 l2 l3 real;

A12 = k1(1) * l1^2 + (k1(2) * l2 + k1(3)) * l1 + (k1(4) * l2^2 + k1(5) * l2 + k1(6));
A23 = k2(1) * l1^2 + (k2(2) * l3 + k2(3)) * l1 + (k2(4) * l3^2 + k2(5) * l3 + k2(6));
A13 = k3(1) * l2^2 + (k3(2) * l3 + k3(3)) * l2 + (k3(4) * l3^2 + k3(5) * l3 + k3(6));

A2 = feval(symengine,'polylib::resultant',A12,A23,l1);
A3 = feval(symengine,'polylib::resultant',A2,A13,l2);
coef = feval(symengine, 'coeff', A3,l3);
ccode(coef(1),'file','getCoefficiantsL3_1.h')
ccode(coef(2),'file','getCoefficiantsL3_2.h')
ccode(coef(3),'file','getCoefficiantsL3_3.h')
ccode(coef(4),'file','getCoefficiantsL3_4.h')
ccode(coef(5),'file','getCoefficiantsL3_5.h')
ccode(coef(6),'file','getCoefficiantsL3_6.h')
ccode(coef(7),'file','getCoefficiantsL3_7.h')
ccode(coef(8),'file','getCoefficiantsL3_8.h')
ccode(coef(9),'file','getCoefficiantsL3_9.h')
