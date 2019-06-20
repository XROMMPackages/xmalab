clear;
%load data, e.g. easyWandData
load 'twoCamCal_v2_easyWandData.mat'

%for each camera
for i = 1:easyWandData.nCams
    %first convert coefs to a 3 by 4 projection
    a = easyWandData.coefs(:,i);
    a = [a' 1];
    a = reshape(a , 4,3)';
    
    %convert it into K, R and t for XMALab
    [K, R, t] = convertPtoXMA(a,true,easyWandData.imageHeight(i));
    
    %save the Mayacam
    filename = ['Cam' int2str(i) '.txt'];
    saveMayaCam(filename , [easyWandData.imageWidth(i),easyWandData.imageHeight(i)],K,R,t);
end