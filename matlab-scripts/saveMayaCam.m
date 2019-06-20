%SAVEMAYACAM Saves a Mayacam file for XMALab using the provided parameters
%
% Usage:  savemayacam(outputfile, imagesize,K,R,t, dist);
%
%    Saves a mayacam for XMALab 
%
% Argument:  outputfile - Filename where to save the mayacam
%            imagesize - 1 x 2 vector containing width and height
%            K - 3 x 3 intrinsic camera matrix
%            R - 3 x 3 camera rotation matrix
%            t - 1 x 3 vector of camera translation
%            dist - optional distortion parameters

function saveMayaCam(outputfile, imagesize,K,R,t, dist)
    fileID = fopen(outputfile,'w');
    fprintf(fileID,'image size\n');
    fprintf(fileID,'%d,%d\n\n',imagesize(1),imagesize(2));

    fprintf(fileID,'camera matrix\n');
    fprintf(fileID,'%.12f,%.12f,%.12f\n%.12f,%.12f,%.12f\n%.12f,%.12f,%.12f\n\n',K(1,1),K(1,2),K(1,3) ...     
            ,K(2,1), K(2,2), K(2,3)  ...
           ,K(3,1),K(3,2),K(3,3) );
       
    fprintf(fileID,'rotation\n');
    fprintf(fileID,'%.12f,%.12f,%.12f\n%.12f,%.12f,%.12f\n%.12f,%.12f,%.12f\n\n', ...
        R(1,1),R(1,2),R(1,3), ...
        R(2,1),R(2,2),R(2,3), ...
        R(3,1),R(3,2),R(3,3) ...    
    );

    fprintf(fileID,'translation\n');
    fprintf(fileID,'%.12f\n%.12f\n%.12f\n\n', ...
        t(1),t(2),t(3));
    
     if exist('dist','var')
        dist = [dist, zeros(1, 8 - length(dist))];
        fprintf(fileID,'undistortion\n');
        fprintf(fileID,'%.12f,%.12f,%.12f,%.12f,%.12f,%.12f,%.12f,%.12f\n'...
        ,dist(0),dist(1),dist(2),dist(3) ...
        ,dist(4),dist(5),dist(6),dist(7) ...
        );
    end
    fclose(fileID);
end

