#pragma once
#include <string>
namespace gt4 {
inline bool previewPlateName(const char*name){
 if(!name)return false;std::string s;for(size_t n=0;n<64&&name[n];++n){const auto c=static_cast<unsigned char>(name[n]);s+=c>='A'&&c<='Z'?char(c+32):char(c);}
 for(const char*p:{"carplate","carpback","careplate","plateback1","plateback2","plateback3","licenseplate","licenceplate","numberplate","license_plate","licence_plate","number_plate","plate","plates","plate_front","plate_rear","platefront","platerear"})if(s==p)return true;
 return s.rfind("license_plate_",0)==0||s.rfind("licence_plate_",0)==0||s.rfind("number_plate_",0)==0;
}
inline bool blankPreviewPlate(const char*texture,const char*node,bool nativeTexture=false,bool nativeMaterial=false){return nativeTexture||nativeMaterial||previewPlateName(texture)||previewPlateName(node);}
}
