clear all;

%pt3 0 : -0.528623 , 4.81039 , -0.115899
%pt3 1 : -0.268667 , 6.98932 , -4.03204
pt3 = [-2.09001 , 6.6483 , -4.55584];

syms l1 real;
syms l2 real;
syms l3 real;

dist12 = 4.48904;
dist13 = 5.05262;
dist23 = 1.9256;

Q1 = [-0.995085, -0.0873616, 0.0466233, 87.1107, 12.5045, -4.22211];
Q2 = [ 0.386456, -0.830332, -0.401498, -28.892, 68.489, 25.7054];
Q3 = [ -0.997845, -0.0655106, -0.00373324, 87.1107, 12.5045, -4.22211];

XG1 = Q1(4:6) + l1 * Q1(1:3);
XG2 = Q2(4:6) + l2 * Q2(1:3);
XG3 = Q3(4:6) + l3 * Q3(1:3);

R = (XG1(1) - XG2(1))^2 + (XG1(2) - XG2(2))^2 + (XG1(3) - XG2(3))^2 - dist12^2;
coef = feval(symengine, 'coeff', R,l1)';
k1(1) = coef(1);
coef2 = feval(symengine, 'coeff', coef(2),l2,All)';
k1(2) = coef2(1);
k1(3) = coef2(2);
coef2 = feval(symengine, 'coeff', coef(3),l2,All)';
k1(4) = coef2(1);
k1(5) = coef2(2);
k1(6) = coef2(3);

R = (XG1(1) - XG3(1))^2 + (XG1(2) - XG3(2))^2 + (XG1(3) - XG3(3))^2 - dist13^2;
coef = feval(symengine, 'coeff', R,l1)';
k2(1) = coef(1);
coef2 = feval(symengine, 'coeff', coef(2),l3,'All')';
k2(2) = coef2(1);
k2(3) = coef2(2);
coef2 = feval(symengine, 'coeff', coef(3),l3,'All')';
k2(4) = coef2(1);
k2(5) = coef2(2);
k2(6) = coef2(3);

R = (XG2(1) - XG3(1))^2 + (XG2(2) - XG3(2))^2 + (XG2(3) - XG3(3))^2 - dist23^2;
coef = feval(symengine, 'coeff', R,l2)';
k3(1) = coef(1);
coef2 = feval(symengine, 'coeff', coef(2),l3)';
k3(2) = coef2(1);
k3(3) = coef2(2);
coef2 = feval(symengine, 'coeff', coef(3),l3)';
k3(4) = coef2(1);
k3(5) = coef2(2);
k3(6) = coef2(3);

clear all;
syms l1 l2 l3 real;
%k1 = [1, 0.661472, -223.874, 1, -206.663, 17466.4]
%k2 = [1, -1.99698, 0, 1, 0, -25.5289]
%k3 = [1, 0.659457, -206.663, 1, -223.947, 17482.9]

k1 = [1, 0.931769, -223.839, 1, -228.696, 17466.4]
k2 = [1, -1.99669, 0, 1, 0, -25.5289]
k3 = [1, 0.918127, -228.696, 1, -222.995, 17482.9]

A12 = k1(1) * l1^2 + (k1(2) * l2 + k1(3)) * l1 + (k1(4) * l2^2 + k1(5) * l2 + k1(6));
A23 = k2(1) * l1^2 + (k2(2) * l3 + k2(3)) * l1 + (k2(4) * l3^2 + k2(5) * l3 + k2(6));
A13 = k3(1) * l2^2 + (k3(2) * l3 + k3(3)) * l2 + (k3(4) * l3^2 + k3(5) * l3 + k3(6));

A2 = feval(symengine,'polylib::resultant',A12,A23,l1);
A3 = feval(symengine,'polylib::resultant',A2,A13,l2);
coef = feval(symengine, 'coeff', A3,l3);