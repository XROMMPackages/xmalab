function [ f ] = computeError2( x )
%COMPUTEERROR2 Summary of this function goes here
%   Detailed explanation goes here
    global pts3d;
    global pts2d;
    global inlier;
    global cam;
    global set;
    global nbImages;

    Rcam(1,:,:) = Rodrigues(x(1:3));
    tcam(1,:) = x(4:6);
    K(1,:,:)= eye(3);
    K(1,1,1) = x(7);
    K(1,2,2) = x(8);
    K(1,1,3) = x(9);
    K(1,2,3) = x(10);
    
    Rcam(2,:,:) = Rodrigues(x(11:13));
    tcam(2,:) = x(14:16);
    K(2,:,:)= eye(3);
    K(2,1,1) = x(17);
    K(2,2,2) = x(18);
    K(2,1,3) = x(19);
    K(2,2,3) = x(20);
    
    Rcam(3,:,:) = Rodrigues(x(21:23));
    tcam(3,:) = x(24:26);
    K(3,:,:)= eye(3);
    K(3,1,1) = x(27);
    K(3,2,2) = x(28);
    K(3,1,3) = x(29);
    K(3,2,3) = x(30);
    
   
    R = zeros(3 * nbImages, 3, 3);
    t = zeros(3 * nbImages, 3);
    
    count = 0;
    for  i = 0:nbImages-1
        count =  count +1;
       [R_tmp , t_tmp]= multTrans( squeeze(Rcam(1,:,:)) , squeeze(tcam(1,:)) , Rodrigues(x((31 + i * 6) : (33 + i *6))) , x((34 + i * 6) :(36 + i *6))) ;
        R(count,:,:) = R_tmp;
        t(count,:) = t_tmp;
    end
    for  i = 0:nbImages-1
        count =  count +1;
       [R_tmp , t_tmp] = multTrans( squeeze(Rcam(2,:,:)) , squeeze(tcam(2,:)) , Rodrigues(x((31 + i * 6) : (33 + i *6))) , x((34 + i * 6) :(36 + i *6))) ;
        R(count,:,:) =  R_tmp;
        t(count,:) = t_tmp;
    end
    for  i = 0:nbImages-1
       count =  count +1;
       [R_tmp , t_tmp] = multTrans( squeeze(Rcam(3,:,:)) , squeeze(tcam(3,:)) , Rodrigues(x((31 + i * 6) : (33 + i *6))) , x((34 + i * 6) :(36 + i *6))) ;
        R(count,:,:) = R_tmp;
        t(count,:) = t_tmp;
    end

    
    count = 1;
    f = zeros(sum(sum(inlier)),1);
    for idx = 1:size(inlier,1)
        for pt_idx = 1:size(inlier,2)
            if inlier(idx,pt_idx) > 0
                pt_uvw = squeeze(K (cam(idx),:,:))* (squeeze(R(idx,:,:)) * squeeze(pts3d(idx,pt_idx,:)) + t(idx,:)');
                pt_uv = pt_uvw(1:2) ./ pt_uvw(3);
                %f(2*count + 1 : 2*count+2,1) = squeeze(pts2d(idx,pt_idx,:)) - pt_uv;
                f(count) = norm(squeeze(pts2d(idx,pt_idx,:)) - pt_uv);
                count = count + 1;
            end
        end
    end
   % [mean(f.*f)  max(f)]
end

