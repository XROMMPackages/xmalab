clear all;
syms dist12 dist13 dist23 real;
syms l real;
Q = sym('Q',[6,1],'real');
pt3d = sym('pt3d',[3,1],'real');


XG = Q(4:6) + l * Q(1:3);
 
R = (XG(1) - pt3d(1))^2 + (XG(2) - pt3d(2))^2 + (XG(3) - pt3d(3))^2 - dist12^2;
a = solve(R,l)


ccode(a,'file','computePoint.h')


