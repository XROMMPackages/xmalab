syms fx fy cx  cy %Camera Params
syms k1 k2 p1 p2 k3 %Camera Params
syms RxCam RyCam RzCam %Camera Rotation
syms txCam tyCam tzCam %Camera translation

syms RxChess RyChess RzChess %Camera Rotation
syms txChess tyChess tzChess %Camera translation

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

%Setup Chess
wxChess = [  0   -RzChess  RyChess;
           RzChess   0   -RxChess;
          -RyChess  RxChess   0   ];
      
R1_normChess = sqrt(RxChess^2 + RyChess^2 + RzChess^2);
RChess = eye(3) + sin(R1_normChess)/R1_normChess*wxChess + (1-cos(R1_normChess))/R1_normChess^2*wxChess^2;
tChess = [txChess ; tyChess ;tzChess];

%Setup Points
p3d = [X ; Y ; Z];

%  Reproject
p3d_proj =  (RCam * ((RChess * p3d) + tChess) + tCam);
x = p3d_proj(1)./p3d_proj(3);
y = p3d_proj(2)./p3d_proj(3);

x2 = x*x;
y2 = y*y;
%r2 = x2 + y2;
xy2 = 2*x*y;

kr = (1 + ((k3*r2 + k2)*r2 + k1)*r2);
up = fx*(x*kr + p1*xy2 + p2*(r2 + 2*x2)) + cx;
vp = fy*(y*kr + p1*(r2 + 2*y2) + p2*xy2) + cy;

f = norm([u; v] - [up ;vp],2);
matlabFunction(f,'File','projFuncWithDistortion.m');
ccode(f,'file','projFuncWithDistortion.h')

J = jacobian(f,[RxCam, RyCam, RzCam, txCam, tyCam, tzCam, fx, fy, cx, cy, k1 ,k2, p1, p2, k3, RxChess, RyChess, RzChess, txChess, tyChess, tzChess] );
matlabFunction(J,'File','jacFuncWithDistortion.m');
ccode(J,'file','jacFuncWithDistortion.h');