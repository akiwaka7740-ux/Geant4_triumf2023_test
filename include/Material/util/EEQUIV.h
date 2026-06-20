/* double ParH[6] = { */
/*   -8.0000, -0.1000, 0.9, 0.95, */
/*    0.0735,  0.0787  */
/* }; */
double ParH[7] = {
  -0.822007   ,
  1.45584     ,
  0.120082    ,
  -0.105702   ,
  -0.00493804 ,
  0.00827138  ,
  0.0
};
double ParHe[4] = {
  -5.9000, -0.0650, 1.01, 0.41
};
double ParC[7] = {
  -1.73081    ,
  0.634957    ,
  -0.049822   ,
  0.106174    ,
  0.042542    ,
  -0.00837297 ,
  -0.00391898  
};
//////////////////////////////////////////////////////
double EEQUIV(double Erecoil/*MeV*/, unsigned int Ztarget) {
  double Elight = 0;
  if( Ztarget==1 /* proton */ ) {
    /* Elight = (Erecoil>0.35)  */
    /*   ? ParH[0]*(1.-exp(ParH[1]*pow(Erecoil,ParH[2])))+ParH[3]*Erecoil  */
    /*   : ParH[4]*Erecoil+ParH[5]*pow(Erecoil,4); // cecil's ver. */
    /* Elight = pow( 10.0, (1.3331*log10(Erecoil)-0.7476) ); // Burks's ver. */
    Elight = pow( 10.0, 
        ParH[0] + 
        ParH[1]*log10( Erecoil ) +
        ParH[2]*pow( log10( Erecoil ) , 2 ) +
        ParH[3]*pow( log10( Erecoil ) , 3 ) +
        ParH[4]*pow( log10( Erecoil ) , 4 ) +
        ParH[5]*pow( log10( Erecoil ) , 5 ) +
        ParH[6]*pow( log10( Erecoil ) , 6 ) );
  }
  if( Ztarget==2 /* Helium */ ) {
    Elight = ParHe[0]*(1.-exp(ParHe[1]*pow(Erecoil,ParHe[2])))+ParHe[3]*Erecoil; // cecil's ver.
  }
  if( Ztarget==6 /* Carbon */ ) {
    /* Elight = 0.017*Erecoil; // cecil's ver. */
    Elight = pow( 10.0, 
        ParC[0] + 
        ParC[1]*log10( Erecoil ) +
        ParC[2]*pow( log10( Erecoil ) , 2 ) +
        ParC[3]*pow( log10( Erecoil ) , 3 ) +
        ParC[4]*pow( log10( Erecoil ) , 4 ) +
        ParC[5]*pow( log10( Erecoil ) , 5 ) +
        ParC[6]*pow( log10( Erecoil ) , 6 ) );
  }
  return Elight; /*MeVee*/
}

