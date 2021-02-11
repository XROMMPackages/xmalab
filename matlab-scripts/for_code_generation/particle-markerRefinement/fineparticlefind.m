%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% function to find particles (fine)
% - now we have radiusrough, calculate again using gaussian weighting
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    function [centrex,centrey,J,eccentricity,brightness,rotation,skewness,radius,brightnesspeak]=fineparticlefind(centreroughx,centreroughy,radiusrough,frame)  %#codegen
    centrex=0;
    centrey=0;
    J=0;
    eccentricity=0;
    brightness=0;
    brightnesspeak=0;
    rotation=0;
    skewness=0;
    radius=0;
    subpixpeak = 0;
    limmult=1.6; %multiplies size of box around particle for fitting -- not a sensitive parameter - should be slightly over one... say 1.6
    maskmult=1; %multiplies fall-off of weighting exponential in fine fit  -- should be about 1.0
    improverthresh=0.5; %repeat centre refinement cycle if x or y correction is greater than improverthresh
    
        % repeat subpixel correction until satisfactory
        doextracycles=3;
        stillgood=true;
        refinementcount=0;
        maxrefinements=limmult*radiusrough/improverthresh; %if it takes more than maxrefinements to find centre correction, then move on
        while stillgood && doextracycles
            refinementcount=refinementcount+1;
            [p,quadric,frameypxp,weightexp,xpp2,ypp2]=polyfitgaussweight(centreroughx,centreroughy,radiusrough,frame,limmult,maskmult);

            %calculate centre based on quadric part p(1:6)
            %note p = [p00 p10 p01 p20 p11 p02 p30 p21 p12 p03...]
            %but following Wolfram Mathworld's ellipse equation: a=p20 b=p11/2 c=p02 d=p10/2 f=p01/2 g=p00
            a=p(4); b=p(5)/2; c=p(6); d=p(2)/2; f=p(3)/2; g=p(1);
            J=a*c-b^2;
            
            %choose method for subpixel calculation
            %use peak of fitted surface
            xc=(b*f-c*d)/J;
            yc=(b*d-a*f)/J;
            %refine with 3rd order terms using 1st order perturbation
            %d1=d+(3*p(7)*xc^2 + 2*p(8)*xc*yc + p(9)*yc^2)/2;
            %f1=f+(p(8)*xc^2 + 2*p(9)*xc*yc + 3*p(10)*yc^2)/2;
            %refine with 3rd, 4th order terms using 1st order perturbation
            %d1=d+(3*p(7)*xc^2 + 2*p(8)*xc*yc + p(9)*yc^2 + 4*p(11)*xc^3 + 3*p(12)*xc^2*yc + 2*p(13)*xc*yc^2 + p(14)*yc^3)/2;
            %f1=f+(p(8)*xc^2 + 2*p(9)*xc*yc + 3*p(10)*yc^2 + p(12)*xc^3 + 2*p(13)*xc^2*yc + 3*p(14)*xc*yc^2 + 4*p(15)*yc^3)/2;
            %xc=(c*d1-b*f1)/(b^2-a*c);
            %yc=(a*f1-b*d1)/(b^2-a*c);
        
            centreroughx=centreroughx+sign(xc)*min([abs(xc) improverthresh]);
            centreroughy=centreroughy+sign(yc)*min([abs(yc) improverthresh]);

            rotation=0.5*acot((c-a)/2/b)+(a-c<0)*pi/2;
            ct=cos(rotation);
            st=sin(rotation);
            P1=p(11)*ct^4-p(12)*ct^3*st+p(13)*ct^2*st^2-p(14)*ct*st^3+p(15)*st^4;
            P2=p(11)*st^4+p(12)*st^3*ct+p(13)*st^2*ct^2+p(14)*st*ct^3+p(15)*ct^4;
            Q1=p(4)*ct^2-p(5)*ct*st+p(6)*st^2;
            Q2=p(4)*st^2+p(5)*st*ct+p(6)*ct^2;
            %radiusrough(i)=sqrt(mean([-Q1/6/P1 -Q2/6/P2]));
            radiusrough=abs(sqrt(sqrt(Q1*Q2/P1/P2/36))); %geometric mean
            
            % if |xc| or |yc| greater than improverthresh, move centrerough by up to a pixel and calculate again
            % stop recalculating if J becomes negative or if finding is taking too long
            stillgood=(refinementcount<=maxrefinements)&&(J>0); %if not still good, stop at once... 
            improverswitch=(abs(xc)>improverthresh)||(abs(yc)>improverthresh); % check if xc,yc above thresh or extra cycles required
            doextracycles = doextracycles - ~improverswitch; %if still good and improverswitch turns off, do extra cycles
        
            
            radius=radiusrough;
        if ~stillgood || ~doextracycles
        %decide limits of particle by when intensity is sufficiently different from quadric, and quadric>0
        %inparticle=and(frameypxp<(polyval2(p(1:6),xcorrection,ycorrection)+quadric)/1.9, quadric>0);
        %inparticle=and(frameypxp<(polyval2(p(1:6),xcorrection,ycorrection)+3*quadric)/4, quadric>0);
        %inparticle=and((frameypxp-quadric)<2, quadric>0);

        %pixradiisquare=(xpp2(inparticle)-xcorrection).^2+(ypp2(inparticle)-ycorrection).^2;
        %radius(i)=sqrt(sum(sum(pixradiisquare.*frame(inparticle)))/brightness(i)); %weighted by the brightness of the pixel - prob better for finding peak
        %radius(i)=sqrt(sum(sum(pixradiisquare.^2))/sum(sum(pixradiisquare))); %weighted by the distance to the pixel - prob better for finding edge    
        %radius(i)=sqrt(abs(a+c)/sum(abs(p(7:15))));
        %radius(i)=sqrt(J(i)/mean(abs(p(11:15)./[1 1 1 1 1])));
        %radius(i)=sqrt(mean([-a/(6*p(11)+3*p(12)+p(13)) -c/(6*p(15)+3*p(14)+p(13))]));
        %radius(i)=sqrt(mean([-a/6/p(11) -c/6/p(15)]));
        semiaxes=sort(sqrt(2*(a*f^2+c*d^2+g*b^2-2*b*d*f-a*c*g)/-J./([c-a a-c]*sqrt(1+4*b^2/(a-c)^2)-c-a)));
        eccentricity=sqrt(1-semiaxes(1)^2/semiaxes(2)^2);
        
        xedge=pol2cart(0,2*radius./sqrt(cos(rotation).^2+sin(rotation).^2/(1-eccentricity.^2))/(1+sqrt(1-eccentricity^2)));
        inparticle=quadric>a*xedge^2+2*d*xedge+g;

        if ~subpixpeak
            %calculate sub-pixel corrections based on centroid of image within particle
            weight=double(frameypxp).*inparticle.*weightexp;
            sumsumweight=sum(sum(weight));
            xc=sum(sum(xpp2.*weight))/sumsumweight;
            yc=sum(sum(ypp2.*weight))/sumsumweight;
        end

        centrex=centreroughx+xc;
        centrey=centreroughy+yc;
        %centrex(i)=centreroughx(i)+min([xc xc2]);
        %centrey(i)=centreroughy(i)+min([yc yc2]);

        %brightness = mean of fitted surface in particle, weighted by brightness at each pixel

        %the third order terms are all skewed => use as rough measure of skewness, defined as ratio of cubic to quadric terms at x,y=radius
        %skewness(i)=sum(abs(p(7:10)))*radius(i); 
        skewness=sum(abs(p(7:10)))*radius/J; 
        end  
        end
        
        

    end
