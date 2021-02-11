syms l real;
Q = sym('Q',[6,1],'real');
XG = Q(4:6) + l * Q(1:3);
ccode(XG','file','compute3DFromPlucker.h')