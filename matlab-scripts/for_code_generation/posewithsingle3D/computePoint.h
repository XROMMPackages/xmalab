  t2 = Q1*Q1;
  t3 = Q2*Q2;
  t4 = Q3*Q3;
  t5 = Q4*Q4;
  t6 = Q6*Q6;
  t7 = Q5*Q5;
  t8 = dist12*dist12;
  t9 = pt3d1*pt3d1;
  t10 = pt3d3*pt3d3;
  t11 = pt3d2*pt3d2;
  t12 = t2+t3+t4;
  t13 = 1.0/t12;
  t14 = t2*t8;
  t15 = t3*t8;
  t16 = t4*t8;
  t17 = Q4*pt3d1*t3*2.0;
  t18 = Q5*pt3d2*t2*2.0;
  t19 = Q4*pt3d1*t4*2.0;
  t20 = Q6*pt3d3*t2*2.0;
  t21 = Q5*pt3d2*t4*2.0;
  t22 = Q6*pt3d3*t3*2.0;
  t23 = Q1*Q2*Q4*Q5*2.0;
  t24 = Q1*Q3*Q4*Q6*2.0;
  t25 = Q2*Q3*Q5*Q6*2.0;
  t26 = Q1*Q2*pt3d1*pt3d2*2.0;
  t27 = Q1*Q3*pt3d1*pt3d3*2.0;
  t28 = Q2*Q3*pt3d2*pt3d3*2.0;
  t29 = t14+t15+t16+t17+t18+t19+t20+t21+t22+t23+t24+t25+t26+t27+t28-t2*t6-t3*t5-t2*t7-t3*t6-t4*t5-t4*t7-t2*t10-t3*t9-t2*t11-t3*t10-t4*t9-t4*t11-Q1*Q2*Q4*pt3d2*2.0-Q1*Q2*Q5*pt3d1*2.0-Q1*Q3*Q4*pt3d3*2.0-Q1*Q3*Q6*pt3d1*2.0-Q2*Q3*Q5*pt3d3*2.0-Q2*Q3*Q6*pt3d2*2.0;
  t30 = sqrt(t29);
  t31 = Q1*pt3d1;
  t32 = Q2*pt3d2;
  t33 = Q3*pt3d3;
  A0[0][0] = t13*(t30+t31+t32+t33-Q1*Q4-Q2*Q5-Q3*Q6);
  A0[1][0] = -t13*(t30-t31-t32-t33+Q1*Q4+Q2*Q5+Q3*Q6);
