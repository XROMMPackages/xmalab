clear all;
k = sym('k',[6,1],'real');
syms l real;

a = k(1);
b = k(2) * l + k(3);
c = k(4)*l^2  + k(5) * l + k(6);

l_out(1) = 1/(2*a) * ( - b + (b^2 -4*a*c)^0.5);
l_out(2) = 1/(2*a) * ( - b - (b^2 -4*a*c)^0.5);

ccode(l_out','file','getL.h')
