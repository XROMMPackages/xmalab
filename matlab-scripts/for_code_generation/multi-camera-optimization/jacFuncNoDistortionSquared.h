  t2 = RxCam*RxCam;
  t3 = RyCam*RyCam;
  t4 = RzCam*RzCam;
  t5 = t2+t3+t4;
  t6 = sqrt(t5);
  t7 = RxChess*RxChess;
  t8 = RyChess*RyChess;
  t9 = RzChess*RzChess;
  t10 = t7+t8+t9;
  t11 = sqrt(t10);
  t12 = sin(t11);
  t13 = 1.0/sqrt(t10);
  t14 = cos(t11);
  t15 = t14-1.0;
  t16 = 1.0/t10;
  t17 = sin(t6);
  t18 = 1.0/sqrt(t5);
  t19 = cos(t6);
  t20 = t19-1.0;
  t21 = 1.0/t5;
  t22 = RzChess*t12*t13;
  t23 = RxChess*RyChess*t15*t16;
  t24 = RyChess*t12*t13;
  t25 = RxChess*t12*t13;
  t26 = RyChess*RzChess*t15*t16;
  t27 = RxCam*t17*t18;
  t28 = RxChess*RzChess*t15*t16;
  t29 = t24+t28;
  t30 = t25-t26;
  t31 = Y*t30;
  t32 = t7+t8;
  t33 = t15*t16*t32;
  t34 = t33+1.0;
  t35 = Z*t34;
  t61 = X*t29;
  t36 = t31+t35-t61+tzChess;
  t37 = t22+t23;
  t38 = t8+t9;
  t39 = t15*t16*t38;
  t40 = t39+1.0;
  t41 = X*t40;
  t42 = t22-t23;
  t43 = X*t42;
  t44 = t25+t26;
  t45 = t7+t9;
  t46 = t15*t16*t45;
  t47 = t46+1.0;
  t48 = Y*t47;
  t57 = Z*t44;
  t49 = t43+t48-t57+tyChess;
  t50 = RyCam*t17*t18;
  t51 = RxCam*RzCam*t20*t21;
  t52 = t50+t51;
  t53 = t24-t28;
  t54 = Z*t53;
  t63 = Y*t37;
  t55 = t41+t54-t63+txChess;
  t56 = RyCam*RzCam*t20*t21;
  t58 = t2+t3;
  t59 = t20*t21*t58;
  t60 = t59+1.0;
  t62 = t36*t60;
  t64 = t27-t56;
  t65 = t49*t64;
  t74 = t52*t55;
  t66 = t62+t65-t74+tzCam;
  t67 = 1.0/pow(t5,3.0/2.0);
  t68 = 1.0/(t5*t5);
  t69 = t2+t4;
  t70 = t17*t18;
  t71 = t2*t19*t21;
  t72 = RxCam*RyCam*RzCam*t17*t67;
  t73 = RxCam*RyCam*RzCam*t20*t68*2.0;
  t75 = 1.0/t66;
  t76 = cy*t66;
  t77 = t27+t56;
  t78 = RzCam*t17*t18;
  t100 = RxCam*RyCam*t20*t21;
  t79 = t78-t100;
  t80 = t55*t79;
  t81 = t20*t21*t69;
  t82 = t81+1.0;
  t83 = t49*t82;
  t134 = t36*t77;
  t84 = t80+t83-t134+tyCam;
  t85 = fy*t84;
  t86 = t76+t85;
  t87 = RxCam*t17*t58*t67;
  t88 = RxCam*t20*t58*t68*2.0;
  t96 = RxCam*t20*t21*2.0;
  t89 = t87+t88-t96;
  t90 = RxCam*RyCam*t17*t67;
  t91 = RzCam*t2*t17*t67;
  t92 = RzCam*t2*t20*t68*2.0;
  t102 = RzCam*t20*t21;
  t103 = RxCam*RyCam*t19*t21;
  t93 = t90+t91+t92-t102-t103;
  t94 = t55*t93;
  t95 = t2*t17*t67;
  t97 = t70+t71+t72+t73-t95;
  t98 = t49*t97;
  t101 = t36*t89;
  t99 = t94+t98-t101;
  t104 = t3+t4;
  t105 = RxCam*RzCam*t19*t21;
  t106 = RyCam*t2*t17*t67;
  t107 = RyCam*t2*t20*t68*2.0;
  t108 = cx*t66;
  t109 = t78+t100;
  t110 = t50-t51;
  t111 = t36*t110;
  t112 = t20*t21*t104;
  t113 = t112+1.0;
  t114 = t55*t113;
  t119 = t49*t109;
  t115 = t111+t114-t119+txCam;
  t116 = fx*t115;
  t117 = t108+t116;
  t118 = 1.0/(t66*t66);
  t140 = t75*t117;
  t120 = -t140+u;
  t121 = t3*t17*t67;
  t122 = RyCam*t17*t58*t67;
  t123 = RyCam*t20*t58*t68*2.0;
  t130 = RyCam*t20*t21*2.0;
  t124 = t122+t123-t130;
  t125 = RzCam*t3*t17*t67;
  t126 = RzCam*t3*t20*t68*2.0;
  t127 = -t90-t102+t103+t125+t126;
  t128 = t49*t127;
  t129 = t3*t19*t21;
  t131 = -t70+t72+t73+t121-t129;
  t132 = t55*t131;
  t136 = t36*t124;
  t133 = t128+t132-t136;
  t156 = t75*t86;
  t135 = -t156+v;
  t137 = RyCam*RzCam*t17*t67;
  t138 = RxCam*t3*t17*t67;
  t139 = RxCam*t3*t20*t68*2.0;
  t141 = RyCam*RzCam*t19*t21;
  t142 = RxCam*t4*t17*t67;
  t143 = RxCam*t4*t20*t68*2.0;
  t144 = RxCam*RzCam*t17*t67;
  t145 = RzCam*t17*t58*t67;
  t146 = RzCam*t20*t58*t68*2.0;
  t147 = t145+t146;
  t155 = RxCam*t20*t21;
  t148 = t137-t141+t142+t143-t155;
  t149 = t55*t148;
  t150 = RyCam*t4*t17*t67;
  t151 = RyCam*t4*t20*t68*2.0;
  t157 = RyCam*t20*t21;
  t152 = t105-t144+t150+t151-t157;
  t153 = t49*t152;
  t159 = t36*t147;
  t154 = t149+t153-t159;
  t158 = t4*t17*t67;
  t160 = 1.0/pow(t10,3.0/2.0);
  t161 = 1.0/(t10*t10);
  t162 = RxChess*RyChess*t12*t160;
  t163 = RzChess*t7*t12*t160;
  t164 = RzChess*t7*t15*t161*2.0;
  t165 = t12*t13;
  t166 = t7*t14*t16;
  t167 = RxChess*RyChess*RzChess*t12*t160;
  t168 = RxChess*RyChess*RzChess*t15*t161*2.0;
  t169 = RxChess*RzChess*t12*t160;
  t170 = RyChess*t7*t12*t160;
  t171 = RyChess*t7*t15*t161*2.0;
  t172 = RxChess*t12*t38*t160;
  t173 = RxChess*t15*t38*t161*2.0;
  t174 = t172+t173;
  t175 = RxChess*RzChess*t14*t16;
  t176 = RxChess*RyChess*t14*t16;
  t183 = RzChess*t15*t16;
  t177 = -t162+t163+t164+t176-t183;
  t178 = Z*t177;
  t179 = t7*t12*t160;
  t180 = RxChess*t12*t32*t160;
  t181 = RxChess*t15*t32*t161*2.0;
  t186 = RxChess*t15*t16*2.0;
  t182 = t180+t181-t186;
  t184 = -t165-t166+t167+t168+t179;
  t185 = Z*t184;
  t187 = RxChess*t12*t45*t160;
  t188 = RxChess*t15*t45*t161*2.0;
  t196 = RyChess*t15*t16;
  t189 = -t169+t170+t171+t175-t196;
  t190 = X*t189;
  t191 = t165+t166+t167+t168-t179;
  t192 = Y*t191;
  t193 = t162+t163+t164-t176-t183;
  t194 = X*t193;
  t203 = Z*t182;
  t195 = t192+t194-t203;
  t197 = -t186+t187+t188;
  t199 = Y*t197;
  t198 = t185+t190-t199;
  t200 = t169+t170+t171-t175-t196;
  t201 = Y*t200;
  t206 = X*t174;
  t202 = t178+t201-t206;
  t204 = t60*t195;
  t205 = t64*t198;
  t208 = t52*t202;
  t207 = t204+t205-t208;
  t209 = RzChess*t8*t12*t160;
  t210 = RzChess*t8*t15*t161*2.0;
  t211 = t8*t12*t160;
  t212 = RyChess*RzChess*t14*t16;
  t213 = RxChess*t8*t12*t160;
  t214 = RxChess*t8*t15*t161*2.0;
  t215 = t8*t14*t16;
  t216 = t165+t167+t168-t211+t215;
  t217 = Z*t216;
  t218 = RyChess*t12*t38*t160;
  t219 = RyChess*t15*t38*t161*2.0;
  t231 = RyChess*t15*t16*2.0;
  t220 = t218+t219-t231;
  t221 = RyChess*RzChess*t12*t160;
  t228 = RxChess*t15*t16;
  t222 = -t212+t213+t214+t221-t228;
  t223 = Y*t222;
  t243 = X*t220;
  t224 = t217+t223-t243;
  t225 = RyChess*t12*t45*t160;
  t226 = RyChess*t15*t45*t161*2.0;
  t227 = t225+t226;
  t229 = t162-t176-t183+t209+t210;
  t230 = Z*t229;
  t232 = RyChess*t12*t32*t160;
  t233 = RyChess*t15*t32*t161*2.0;
  t234 = -t162+t176-t183+t209+t210;
  t235 = Y*t234;
  t236 = -t165+t167+t168+t211-t215;
  t237 = X*t236;
  t238 = -t231+t232+t233;
  t244 = Z*t238;
  t239 = t235+t237-t244;
  t240 = t212+t213+t214-t221-t228;
  t241 = X*t240;
  t245 = Y*t227;
  t242 = t230+t241-t245;
  t246 = t60*t239;
  t247 = t64*t242;
  t249 = t52*t224;
  t248 = t246+t247-t249;
  t250 = RyChess*t9*t12*t160;
  t251 = RyChess*t9*t15*t161*2.0;
  t252 = t9*t14*t16;
  t253 = RxChess*t9*t12*t160;
  t254 = RxChess*t9*t15*t161*2.0;
  t255 = t9*t12*t160;
  t256 = -t165+t167+t168-t252+t255;
  t257 = Y*t256;
  t258 = RzChess*t12*t38*t160;
  t259 = RzChess*t15*t38*t161*2.0;
  t272 = RzChess*t15*t16*2.0;
  t260 = t258+t259-t272;
  t261 = t212-t221-t228+t253+t254;
  t262 = Z*t261;
  t283 = X*t260;
  t263 = t257+t262-t283;
  t264 = RzChess*t12*t32*t160;
  t265 = RzChess*t15*t32*t161*2.0;
  t266 = t264+t265;
  t267 = -t212+t221-t228+t253+t254;
  t268 = X*t267;
  t269 = -t169+t175-t196+t250+t251;
  t270 = Y*t269;
  t277 = Z*t266;
  t271 = t268+t270-t277;
  t273 = RzChess*t12*t45*t160;
  t274 = RzChess*t15*t45*t161*2.0;
  t275 = t169-t175-t196+t250+t251;
  t276 = Z*t275;
  t278 = t60*t271;
  t279 = t165+t167+t168+t252-t255;
  t280 = X*t279;
  t281 = -t272+t273+t274;
  t284 = Y*t281;
  t282 = t276+t280-t284;
  t285 = t64*t282;
  t287 = t52*t263;
  t286 = t278+t285-t287;
  A0[0][0] = t135*(t75*(cy*t99+fy*(t55*(t105+t106+t107-RyCam*t20*t21-RxCam*RzCam*t17*t67)-t49*(-t96+RxCam*t17*t67*t69+RxCam*t20*t68*t69*2.0)+t36*(-t70-t71+t72+t73+t95)))-t86*t99*t118)*-2.0-t120*(t75*(cx*t99+fx*(t36*(-t90+t91+t92-t102+t103)-t55*(RxCam*t17*t67*t104+RxCam*t20*t68*t104*2.0)+t49*(-t105+t106+t107+t144-RyCam*t20*t21)))-t99*t117*t118)*2.0;
  A0[0][1] = t120*(t75*(fx*(t49*(t137+t138+t139-RxCam*t20*t21-RyCam*RzCam*t19*t21)-t55*(-t130+RyCam*t17*t67*t104+RyCam*t20*t68*t104*2.0)+t36*(t70+t72+t73-t121+t129))+cx*t133)-t117*t118*t133)*-2.0-t135*(t75*(cy*t133+fy*(t36*(t90-t102-t103+t125+t126)-t49*(RyCam*t17*t67*t69+RyCam*t20*t68*t69*2.0)+t55*(-t137+t138+t139+t141-RxCam*t20*t21)))-t86*t118*t133)*2.0;
  A0[0][2] = t120*(t75*(cx*t154+fx*(t49*(-t70+t72+t73+t158-t4*t19*t21)+t36*(-t137+t141+t142+t143-t155)-t55*(RzCam*t20*t21*-2.0+RzCam*t17*t67*t104+RzCam*t20*t68*t104*2.0)))-t117*t118*t154)*-2.0-t135*(t75*(cy*t154+fy*(t55*(t70+t72+t73-t158+t4*t19*t21)+t36*(-t105+t144+t150+t151-t157)-t49*(RzCam*t20*t21*-2.0+RzCam*t17*t67*t69+RzCam*t20*t68*t69*2.0)))-t86*t118*t154)*2.0;
  A0[0][3] = fx*t75*t120*-2.0;
  A0[0][4] = fy*t75*t135*-2.0;
  A0[0][5] = t120*(cx*t75-t117*t118)*-2.0-t135*(cy*t75-t86*t118)*2.0;
  A0[0][6] = t75*t115*t120*-2.0;
  A0[0][7] = t75*t84*t135*-2.0;
  A0[0][8] = u*-2.0+t75*t117*2.0;
  A0[0][9] = v*-2.0+t75*t86*2.0;
  A0[0][10] = t120*(t75*(cx*t207+fx*(t110*t195-t109*t198+t113*t202))-t117*t118*t207)*-2.0-t135*(t75*(cy*t207+fy*(-t77*t195+t82*t198+t79*t202))-t86*t118*t207)*2.0;
  A0[0][11] = t120*(t75*(cx*t248+fx*(t113*t224+t110*t239-t109*t242))-t117*t118*t248)*-2.0-t135*(t75*(cy*t248+fy*(t79*t224-t77*t239+t82*t242))-t86*t118*t248)*2.0;
  A0[0][12] = t120*(t75*(cx*t286+fx*(t113*t263+t110*t271-t109*t282))-t117*t118*t286)*-2.0-t135*(t75*(cy*t286+fy*(t79*t263-t77*t271+t82*t282))-t86*t118*t286)*2.0;
  A0[0][13] = t120*(t75*(cx*t52-fx*t113)-t52*t117*t118)*2.0+t135*(t75*(cy*t52-fy*t79)-t52*t86*t118)*2.0;
  A0[0][14] = t120*(t75*(cx*t64-fx*t109)-t64*t117*t118)*-2.0-t135*(t75*(cy*t64+fy*t82)-t64*t86*t118)*2.0;
  A0[0][15] = t120*(t75*(cx*t60+fx*t110)-t60*t117*t118)*-2.0-t135*(t75*(cy*t60-fy*t77)-t60*t86*t118)*2.0;
