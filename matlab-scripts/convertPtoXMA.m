%CONVERTPTOXMA Converts a projectionmatrix to the XMALab camera model
%
% Usage:  [K,R,t] = convertPtoXMA(P,frommatlab, height);
%
%    P is decomposed into the form P = K*[R t] to be used in XMALab (or OpenCV)
%
% Argument:  P - 3 x 4 camera projection matrix
%            frommatlab - boolean to indicate if the projection was used in
%                        matlab. In this case the pixel coordinates need to
%                        be adjusted to start from 0 instead of 1. In addition 
%                        the image origin is defined in the lower left corner 
%                        in matlab while in XMALab it is the upper left corner 
%                        (default : false)
%            height - image height (only required in case frommatlab is true)
% Returns:   
%            K - Calibration matrix of the form
%                  |  ax   s   ppx |
%                  |   0   ay  ppy |
%                  |   0   0    1  |
%
%                Where: 
%                ax = f/pixel_width and ay = f/pixel_height,
%                ppx and ppy define the principal point in pixels,
%           R - 3 x 3 rotation matrix defining the world coordinate frame
%                in terms of the camera frame. Columns of R transposed define
%                the directions of the camera X, Y and Z axes in world
%                coordinates. 
%           t - Camera centre position in world coordinates.
%
% See also: decomposecamera

function [K,R,t] = convertPtoXMA(P,frommatlab, height)

    if ~exist('frommatlab','var')
        fromMatlab = false;
    end
    if frommatlab
        if ~exist('height','var')
            error('You have to enter the height of the image when converting from matlab')
        end
    end
    
    [K, R, t] = decomposecamera(P);
    
    %normalize K to K(3,3) == 1.
    K = K/K(3,3);
    
    if frommatlab
        %adjust principal in x from 1 to 0 start
        K(1,3) = K(1,3)-1;
    
        %convert principal in y-direction and adjust start from 1 to 0
        K(2,3) = height - K(2,3) + 2;
        
        %invert y-axis for R
        R = [1 0 0 ; 0 -1 0; 0 0 1]*R;
    end
       
    %convert camera position to translation vector
    t = -R *t;
end

