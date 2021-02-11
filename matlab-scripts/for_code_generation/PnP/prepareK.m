clear all;
syms dist12 dist13 dist23 real;
syms l1 real;
syms l2 real;
syms l3 real;
Q1 = sym('Q1',[6,1],'real');
Q2 = sym('Q2',[6,1],'real');
Q3 = sym('Q3',[6,1],'real');

XG1 = Q1(4:6) + l1 * Q1(1:3);
XG2 = Q2(4:6) + l2 * Q2(1:3);
XG3 = Q3(4:6) + l3 * Q3(1:3);
 
R = (XG1(1) - XG2(1))^2 + (XG1(2) - XG2(2))^2 + (XG1(3) - XG2(3))^2 - dist12^2;
coef = feval(symengine, 'coeff', R,l1)';
k1(1) = coef(1);
coef2 = feval(symengine, 'coeff', coef(2),l2)';
k1(2) = coef2(1);
k1(3) = coef2(2);
coef2 = feval(symengine, 'coeff', coef(3),l2)';
k1(4) = coef2(1);
k1(5) = coef2(2);
k1(6) = coef2(3);

R = (XG1(1) - XG3(1))^2 + (XG1(2) - XG3(2))^2 + (XG1(3) - XG3(3))^2 - dist13^2;
coef = feval(symengine, 'coeff', R,l1)';
k2(1) = coef(1);
coef2 = feval(symengine, 'coeff', coef(2),l3)';
k2(2) = coef2(1);
k2(3) = coef2(2);
coef2 = feval(symengine, 'coeff', coef(3),l3)';
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

k = [k1' k2' k3']
ccode(k','file','computeCoefficiants.h')


