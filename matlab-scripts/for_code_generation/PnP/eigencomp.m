clear all;
C = sym('C',[8,1],'real');
syms x;
p =  x^8 + C(1) * x^7 + C(2) * x^6 + C(3) * x^5 + C(4) * x^4 + C(5) * x^3 + C(6) * x^2 + C(7) * x^1 + C(8);
comp = feval(symengine, 'linalg::companion', p, x );
Sol = feval(symengine, 'linalg::eigenvalues',comp)