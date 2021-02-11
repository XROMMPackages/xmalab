clear all;
syms fx fy cx  cy %Camera Params
syms k1 k2 p1 p2 k3 %Camera Params
syms RxCam RyCam RzCam %Camera Rotation
syms txCam tyCam tzCam %Camera translation

syms RxBody RyBody RzBody %Camera Rotation
syms txBody tyBody tzBody %Camera translation

syms X Y Z %3D Point
syms u v %2D  Point

%Setup Cam
K = [fx 0 cx  ;  0 fy cy ; 0 0 1];
wxCam = [  0   -RzCam  RyCam;
           RzCam   0   -RxCam;
          -RyCam  RxCam   0   ];
      
R1_normCam = sqrt(RxCam^2 + RyCam^2 + RzCam^2);
RCam = eye(3) + sin(R1_normCam)/R1_normCam*wxCam + (1-cos(R1_normCam))/R1_normCam^2*wxCam^2;
tCam= [txCam ; tyCam ; tzCam];

%Setup Body
wxBody = [  0   -RzBody  RyBody;
           RzBody   0   -RxBody;
          -RyBody  RxBody   0   ];
      
R1_normBody = sqrt(RxBody^2 + RyBody^2 + RzBody^2);
RBody = eye(3) + sin(R1_normBody)/R1_normBody*wxBody + (1-cos(R1_normBody))/R1_normBody^2*wxBody^2;
tBody = [txBody ; tyBody ;tzBody];

%Setup Points
p3 = [X ; Y ; Z];

%  Reproject
p3d_proj =  (RCam * ((RBody * p3) + tBody) + tCam);

x = p3d_proj(1,1)./p3d_proj(3,1);
y = p3d_proj(2,1)./p3d_proj(3,1);

x2 = x*x;
y2 = y*y;
r2 = x2 + y2;
xy2 = 2*x*y;

kr = (1 + ((k3*r2 + k2)*r2 + k1)*r2);
up = fx*(x*kr + p1*xy2 + p2*(r2 + 2*x2)) + cx;
vp = fy*(y*kr + p1*(r2 + 2*y2) + p2*xy2) + cy;

f = [u; v] - [up ;vp];

matlabFunction(f,'File','projFuncWithDistortionPoseSeperateDimensions.m');
ccode(f,'file','projFuncWithDistortionPoseSeperateDimensions.h')

J = jacobian(f,[RxBody, RyBody, RzBody, txBody, tyBody, tzBody] )
matlabFunction(J,'File','jacFuncWithDistortionPoseSeperateDimensions.m');
ccode(J,'file','jacFuncWithDistortionPoseSeperateDimensions.h')