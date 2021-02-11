%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% function to calculate the Gaussian-weighted polynomial fit of the particle
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    function [p,quadric,frameypxp,weightexp,xpp2,ypp2]=polyfitgaussweight(roughx,roughy,roughradius,frame,limmult,maskmult)  %#codegen  
        %define coordinates around centrerough - (xp,yp) and (xpp,ypp)
        limx=round([max([roughx-limmult*roughradius 1]) min([roughx+limmult*roughradius size(frame,1)])]);
        limy=round([max([roughy-limmult*roughradius 1]) min([roughy+limmult*roughradius size(frame,2)])]);
        xp=limx(1):limx(2);
        xpp=xp-roughx;
        yp=limy(1):limy(2);
        ypp=yp-roughy;
        %fit quartic surface within limits, weighted by exponential around centrerough
        lxpp=length(xpp);
        lypp=length(ypp);
        xpp2=ones(lypp,1)*xpp;
        ypp2=ypp'*ones(1,lxpp);
        weightexp=exp(-(xpp2.^2+ypp2.^2)/roughradius.^2*maskmult);

        frameypxp=frame(yp,xp);
        
        %p=polyfitweighted2(xpp,ypp,frameypxp,4,weightexp.*frameypxp);
        p=polyfitweighted2(xpp,ypp,frameypxp,4,weightexp);

        quadric=polyval2(p(1:6),xpp,ypp);
        %quartic=polyval2(p(1:15),xpp,ypp);

    end