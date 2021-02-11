clear;
clearvars -global;
global pts3d;
global pts2d;
global inlier;
global cam;
global set;
global image_idx;
global nbImages;

folder = 'C:\Workspace\data\vromm\crab\crab\';
folder_out = 'C:\Workspace\data\vromm\crab\crab2\';
checkerdim = [7 6];
%folder = 'C:\Workspace\data\vromm\underwater_checker\underwater_checker\';
%folder_out = 'C:\Workspace\data\vromm\underwater_checker\underwater_checker2\';
%checkerdim = [6 8];
checkersize = 1;

[X,Y] = meshgrid(0:checkersize:checkerdim(1)*checkersize,0:checkersize:checkerdim(2)*checkersize); %Cam3
calib_object(2,:) = reshape(X,[1,size(X,1)*size(X,2)]);
calib_object(1,:) = reshape(Y,[1,size(X,1)*size(X,2)]);
calib_object(3,:) = zeros(1,size(calib_object,2));

%Camera 1
contents1_tmp =  dir(strcat(folder , 'Camera 1\*.jpg'));
name1_tmp = {contents1_tmp(:).name}';
nbImages = size(name1_tmp,1);
count = 0;
K(1,:,:) = csvread([folder 'Camera 1\data\Camera 1_CameraMatrix.csv']);

for  i = 1:nbImages
   count = count + 1;
   name = char(strcat(folder,strcat('Camera 1\',name1_tmp(i))));
   [pathstr1(i,:),name1(i,:),ext1(i,:)] = fileparts(name);
   image_idx(count) = i;
   cam(count) = 1;
   if exist([pathstr1(i,:) '\data\' name1(i,:) '_PointsDetected.csv'], 'file')
       pts2d(count,:,:) = csvread([pathstr1(i,:) '\data\' name1(i,:) '_PointsDetected.csv']);
       pts3d(count,:,:) = calib_object';
       inlier(count,:) = csvread([pathstr1(i,:) '\data\' name1(i,:) '_PointsInlier.csv']);
       rotationMat(count,:,:) = csvread([pathstr1(i,:) '\data\' name1(i,:) '_RotationMatrix.csv']);
       translation(count,:,:) = csvread([pathstr1(i,:) '\data\' name1(i,:) '_TranslationVector.csv']);
       set(count) = 1;
   else
       set(count) = 0;
   end
end
  
%Camera 2
contents2_tmp =  dir(strcat(folder , 'Camera 2\*.jpg'));
name2_tmp = {contents2_tmp(:).name}';
K(2,:,:) = csvread([folder 'Camera 2\data\Camera 2_CameraMatrix.csv']);

for  i = 1:nbImages
   count = count + 1;
   name = char(strcat(folder,strcat('Camera 2\',name2_tmp(i))));
   [pathstr2(i,:),name2(i,:),ext2(i,:)] = fileparts(name);
   cam(count) = 2;
   image_idx(count) = i;
   if exist([pathstr2(i,:) '\data\' name2(i,:) '_PointsDetected.csv'], 'file')
       pts2d(count,:,:) = csvread([pathstr2(i,:) '\data\' name2(i,:) '_PointsDetected.csv']);
       pts3d(count,:,:) = calib_object';
       inlier(count,:) = csvread([pathstr2(i,:) '\data\' name2(i,:) '_PointsInlier.csv']);
       rotationMat(count,:,:) = csvread([pathstr2(i,:) '\data\' name2(i,:) '_RotationMatrix.csv']);
       translation(count,:,:) = csvread([pathstr2(i,:) '\data\' name2(i,:) '_TranslationVector.csv']);
       set(count) = 1;
   else
       set(count) = 0;
   end
   
end

%Camera 3
contents3_tmp =  dir(strcat(folder , 'Camera 3\*.jpg'));
name3_tmp = {contents3_tmp(:).name}';
K(3,:,:) = csvread([folder 'Camera 3\data\Camera 3_CameraMatrix.csv']);

for  i = 1:nbImages
   count = count + 1;
   name = char(strcat(folder,strcat('Camera 3\',name3_tmp(i))));
   [pathstr3(i,:),name3(i,:),ext3(i,:)] = fileparts(name);
   cam(count) = 3;
   image_idx(count) = i;
   if exist([pathstr3(i,:) '\data\' name3(i,:) '_PointsDetected.csv'], 'file')
       pts2d(count,:,:) = csvread([pathstr3(i,:) '\data\' name3(i,:) '_PointsDetected.csv']);
       pts3d(count,:,:) = calib_object';
       inlier(count,:) = csvread([pathstr3(i,:) '\data\' name3(i,:) '_PointsInlier.csv']);
       rotationMat(count,:,:) = csvread([pathstr3(i,:) '\data\' name3(i,:) '_RotationMatrix.csv']);
       translation(count,:,:) = csvread([pathstr3(i,:) '\data\' name3(i,:) '_TranslationVector.csv']);
       set(count) = 1;
   else
       set(count) = 0;
   end
end

for  i = 1:nbImages
    squeeze(rotationMat(i,:,:)) * squeeze(rotationMat(i+nbImages,:,:))';
end 

[meanError sdError]= computeError( K, rotationMat, translation, pts3d,pts2d, cam ,inlier)

R(1,:,:) = squeeze(rotationMat(1,:,:)) * Rodrigues([0.1,0.1,0.1]);
R(2,:,:) = squeeze(rotationMat(nbImages + 1,:,:))* Rodrigues([0.1,0.1,0.1]);
R(3,:,:) = squeeze(rotationMat(2 * nbImages + 1,:,:))* Rodrigues([0.1,0.1,0.1]);

t(1,:) = squeeze(translation(1,:,:));
t(2,:) = squeeze(translation(nbImages + 1,:,:));
t(3,:) = squeeze(translation(2 * nbImages + 1,:,:));


for  i = 1:nbImages
    count = 0;
    R_all = zeros(3,1);
    t_all = zeros(3,1);
    if set(i) > 0
        count = count + 1;
        [R_tmp t_tmp]= multTrans( squeeze(R(1,:,:))' , -(squeeze(R(1,:,:))' * squeeze(t(1,:))')' , squeeze(rotationMat(i,:,:)) ,squeeze(translation(i,:)) );
        R_all = R_all + Rodrigues(R_tmp);
        t_all = t_all + t_tmp;
    end
    if set(nbImages + i) > 0
       count = count + 1;
       [R_tmp t_tmp]= multTrans( squeeze(R(2,:,:))' , -(squeeze(R(2,:,:))' * squeeze(t(2,:))')' , squeeze(rotationMat(nbImages + i,:,:)) ,squeeze(translation(nbImages + i,:)) );
        R_all = R_all + Rodrigues(R_tmp);
        t_all = t_all + t_tmp;
    end
    if set(2*nbImages + i) > 0
       count = count + 1;
       [R_tmp t_tmp]= multTrans( squeeze(R(3,:,:))' , -(squeeze(R(3,:,:))' * squeeze(t(3,:))')' , squeeze(rotationMat(2 * nbImages + i,:,:)) ,squeeze(translation(2 * nbImages + i,:)) );
        R_all = R_all + Rodrigues(R_tmp);
        t_all = t_all + t_tmp;
    end
    if count >0
        Rchess(i,:) = R_all ./ count;
        Tchess(i,:) = t_all ./ count;
    end
end

x0(1:3) = Rodrigues(squeeze(R(1,:,:)));
x0(4:6) = squeeze(t(1,:));
x0(7:8) = [K(1,1,1) K(1,2,2)];
x0(9:10) = K(1,1:2,3);
x0(11:13) = Rodrigues(squeeze(R(2,:,:)));
x0(14:16) = squeeze(t(2,:));
x0(17:18) = [K(2,1,1) K(2,2,2)];
x0(19:20) = K(2,1:2,3);
x0(21:23) = Rodrigues(squeeze(R(3,:,:)));
x0(24:26) = squeeze(t(3,:));
x0(27:28) = [K(3,1,1) K(3,2,2)];
x0(29:30) = K(3,1:2,3);

for  i = 0:nbImages-1
   x0((31 + i * 6) : (33 + i *6)) = Rchess(i+1,:);
   x0((34 + i * 6) :(36 + i *6)) = Tchess(i+1,:);
end
[f1] = computeError2(x0);
[f f2] = computeError3(x0);
mean(f)
std(f)
%options = optimset('Algorithm', {'levenberg-marquardt',.0005}, 'Display','iter','TolFun',1e-6,'TolX',1e-4,'MaxIter',10,'MaxFunEvals',1000);
options = optimset('Algorithm', {'levenberg-marquardt',.0005},'Jacobian','on', 'Display','iter','TolFun',1e-12,'TolX',1e-12,'MaxIter',1000,'MaxFunEvals',10000);
[x,resnorm] = lsqnonlin(@computeError3,x0,[],[],options);
resnorm

f = computeError2(x);
mean(f)
std(f)

Rcam(1,:,:) = Rodrigues(x(1:3));
tcam(1,:) = x(4:6);
K_new(1,:,:)= eye(3);
K_new(1,1,1) = x(7);
K_new(1,2,2) = x(8);
K_new(1,1,3) = x(9);
K_new(1,2,3) = x(10);

Rcam(2,:,:) = Rodrigues(x(11:13));
tcam(2,:) = x(14:16);
K_new(2,:,:)= eye(3);
K_new(2,1,1) = x(17);
K_new(2,2,2) = x(18);
K_new(2,1,3) = x(19);
K_new(2,2,3) = x(20);

Rcam(3,:,:) = Rodrigues(x(21:23));
tcam(3,:) = x(24:26);
K_new(3,:,:)= eye(3);
K_new(3,1,1) = x(27);
K_new(3,2,2) = x(28);
K_new(3,1,3) = x(29);
K_new(3,2,3) = x(30);


R_new = zeros(3 * nbImages, 3, 3);
t_new = zeros(3 * nbImages, 3);

count = 0;
for  i = 0:nbImages-1
    count =  count +1;
   [R_tmp , t_tmp]= multTrans( squeeze(Rcam(1,:,:)) , squeeze(tcam(1,:)) , Rodrigues(x((31 + i * 6) : (33 + i *6))) , x((34 + i * 6) :(36 + i *6))) ;
    R_new(count,:,:) = R_tmp;
    t_new(count,:) = t_tmp;
end
for  i = 0:nbImages-1
    count =  count +1;
   [R_tmp , t_tmp] = multTrans( squeeze(Rcam(2,:,:)) , squeeze(tcam(2,:)) , Rodrigues(x((31 + i * 6) : (33 + i *6))) , x((34 + i * 6) :(36 + i *6))) ;
    R_new(count,:,:) =  R_tmp;
    t_new(count,:) = t_tmp;
end
for  i = 0:nbImages-1
   count =  count +1;
   [R_tmp , t_tmp] = multTrans( squeeze(Rcam(3,:,:)) , squeeze(tcam(3,:)) , Rodrigues(x((31 + i * 6) : (33 + i *6))) , x((34 + i * 6) :(36 + i *6))) ;
    R_new(count,:,:) = R_tmp;
    t_new(count,:) = t_tmp;
end

[meanError sdError]= computeError( K_new, R_new, t_new, pts3d,pts2d, cam ,inlier)
    
%Camera 1
count = 0;

mkdir([folder_out 'Camera 1\data\'])
csvwrite([folder_out 'Camera 1\data\Camera 1_CameraMatrix.csv'],squeeze(K_new(1,:,:)));

for  i = 1:nbImages
   count = count + 1;
   csvwrite([folder_out 'Camera 1\data\' name1(i,:) '_RotationMatrix.csv'],squeeze(R_new(count,:,:)));
   csvwrite([folder_out 'Camera 1\data\' name1(i,:) '_TranslationVector.csv'],squeeze(t_new(count,:))');
end

mkdir([folder_out 'Camera 2\data\'])
csvwrite([folder_out 'Camera 2\data\Camera 2_CameraMatrix.csv'],squeeze(K_new(2,:,:)));

for  i = 1:nbImages
   count = count + 1;
   csvwrite([folder_out '\Camera 2\data\' name2(i,:) '_RotationMatrix.csv'],squeeze(R_new(count,:,:)));
   csvwrite([folder_out '\Camera 2\data\' name2(i,:) '_TranslationVector.csv'],squeeze(t_new(count,:))');
end

mkdir([folder_out 'Camera 3\data\'])
csvwrite([folder_out 'Camera 3\data\Camera 3_CameraMatrix.csv'],squeeze(K_new(3,:,:)));

for  i = 1:nbImages
   count = count + 1;
   csvwrite([folder_out '\Camera 3\data\' name3(i,:) '_RotationMatrix.csv'],squeeze(R_new(count,:,:)));
   csvwrite([folder_out '\Camera 3\data\' name3(i,:) '_TranslationVector.csv'],squeeze(t_new(count,:))');
end
p  = zeros(size(f2));
p(find(f2~=0)) = 1000;
mesh(p)

%pt = squeeze(K (1,:,:))* (squeeze(rotationMat(1,:,:)) * squeeze(Pts3D(:,1)) + translation(1,:)')
