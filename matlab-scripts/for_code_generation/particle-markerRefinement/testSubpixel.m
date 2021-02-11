RGB = imread('C:\Workspace\data\pig\jpg\20061229susDFc01S0001-00Mirroredshorter.avi-JPG\20061229susDFc01S0001-00Mirroredshorter.00001.jpg');
I = rgb2gray(RGB);
I = 255 - I;
pts_all = csvread('C:\Workspace\data\pig\pts2d.csv');
pts1 = pts_all(1,:);
pts1_cam1x = pts1(1:4:end);
pts1_cam1y = pts1(2:4:end);

imshow(I);
hold on;
plot(pts1_cam1x,pts1_cam1y,'rx','MarkerSize',20);

tmpSize = 2.5;
for i=1:size(pts1_cam1x,2)
[x(i) y(i) J(i) eccentricity(i) brightness(i) rotation(i) skewness(i) radius(i) brightnesspeak(i) ] = fineparticlefind(pts1_cam1x(i)  ,pts1_cam1y(i) ,tmpSize,I)
end

plot(x,y,'b+','MarkerSize',20);
hold off;
 