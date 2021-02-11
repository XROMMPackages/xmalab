function [ R t ] = multTrans( R1 , t1 , R2 ,t2 )
%MULTTRANS Summary of this function goes here
%   Detailed explanation goes here
    T1 = [R1 t1' ; 0 0 0 1];
    T2 = [R2 t2' ; 0 0 0 1];
    T = T1 * T2;
    R = T(1:3,1:3);
    t = T(1:3,4);
end

