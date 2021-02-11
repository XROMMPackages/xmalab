function [ meanerror sderror error ] = computeError( K, R, t, PT3,PT2, Cam ,Inlier)
%COMPUTEERROR Summary of this function goes here
%   Detailed explanation goes here
count = 0;
for idx = 1:size(R,1)
    for pt_idx = 1:size(Inlier,2)
        if Inlier(idx,pt_idx) > 0
            count = count + 1;
            pt_uvw = squeeze(K (Cam(idx),:,:))* (squeeze(R(idx,:,:)) * squeeze(PT3(idx,pt_idx,:)) + t(idx,:)');
            pt_uv = pt_uvw(1:2) ./ pt_uvw(3);
            error(count) = norm(squeeze(PT2(idx,pt_idx,:)) - pt_uv);
        end
    end
end

meanerror = mean(error);
sderror = std(error);
end

