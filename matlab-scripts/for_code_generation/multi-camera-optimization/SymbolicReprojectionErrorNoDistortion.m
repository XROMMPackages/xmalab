syms fx fy cx  cy %Camera Params
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
p3 = [X ; Y ; Z];
p2 = [u ; v];

%  Reproject
p3_proj =  K * (RCam * ((RChess * p3) + tChess) + tCam);

f = norm(p2 - p3_proj(1:2)./p3_proj(3),2)
matlabFunction(f,'File','projFuncNoDistortion.m');
ccode(f,'file','projFuncNoDistortion.h')

J = jacobian(f,[RxCam, RyCam, RzCam, txCam, tyCam, tzCam, fx, fy, cx, cy, RxChess, RyChess, RzChess, txChess, tyChess, tzChess] )
matlabFunction(J,'File','jacFuncNoDistortion.m');
ccode(J,'file','jacFuncNoDistortion.h')