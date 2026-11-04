#ifndef XMVSF_STAGE87_74_CALIBRATION_H
#define XMVSF_STAGE87_74_CALIBRATION_H
// Test-only native-pixel offset adjustment. Pure C++11, no emulator state writes.
namespace XM8774Calibration {
struct Offset {
 int x,y;
 unsigned held[4];
 Offset():x(-55),y(0){for(int i=0;i<4;++i)held[i]=0;}
 bool Keys(const bool arrow[4],bool shift){
  bool changed=false;
  for(int k=0;k<4;++k){
   if(!arrow[k]){held[k]=0;continue;}
   const unsigned count=++held[k];
   if(count!=1u && (count<=12u || ((count-12u)%4u)!=0u))continue;
   const int step=shift?8:1;
   const int dx=(k==0?-step:k==1?step:0);
   const int dy=(k==2?-step:k==3?step:0);
   const int nx=x+dx,ny=y+dy;
   if(nx<-160 || nx>160 || ny<-160 || ny>160)continue;
   x=nx;y=ny;changed=true;
  }
  return changed;
 }
};
}
#endif
