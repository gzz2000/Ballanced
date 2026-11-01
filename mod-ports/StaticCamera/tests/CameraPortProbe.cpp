#include <BML/IMod.h>
#include <cstdio>
#include <cstring>
static IBML *api;
class CameraPortProbe: public IMod {
public:
 explicit CameraPortProbe(IBML *a):IMod(a){}
 const char *GetID() override{return "CameraPortProbe";}
 const char *GetVersion() override{return "0.1.0";}
 const char *GetName() override{return "Camera Port Test Observer";}
 const char *GetAuthor() override{return "BallanceWeb tests";}
 const char *GetDescription() override{return "Read camera/fog state and change real mod config properties for automated tests";}
 DECLARE_BML_VERSION;
 void OnLoad() override{api=m_BML;}
 void OnUnload() override{api=nullptr;}
};
MOD_EXPORT IMod *BMLEntry(IBML *a){return new CameraPortProbe(a);}
MOD_EXPORT void BMLExit(IMod *m){delete m;}
MOD_EXPORT const char *BML_WebCameraPortState(){
 static char result[2048];if(!api)return "{}";
 auto *r=api->GetRenderContext();auto *cam=r->GetAttachedCamera();auto *normal=api->GetTargetCameraByName("InGameCam");
 auto *arr=api->GetArrayByName("CurrentLevel");auto *ball=arr?static_cast<CK3dEntity*>(arr->GetElementObject(0,1)):nullptr;
 VxVector c(0,0,0),b(0,0,0),n(0,0,0);if(cam)cam->GetPosition(&c);if(ball)ball->GetPosition(&b);if(normal)normal->GetPosition(&n);
 std::snprintf(result,sizeof(result),"{\"playing\":%s,\"ingame\":%s,\"camera\":\"%s\",\"cam\":[%g,%g,%g],\"ball\":[%g,%g,%g],\"normal\":[%g,%g,%g],\"fog\":%d,\"start\":%g,\"end\":%g,\"color\":%u,\"back\":%g}",api->IsPlaying()?"true":"false",api->IsIngame()?"true":"false",cam?cam->GetName():"",c.x,c.y,c.z,b.x,b.y,b.z,n.x,n.y,n.z,int(r->GetFogMode()),r->GetFogStart(),r->GetFogEnd(),unsigned(r->GetFogColor()),normal?normal->GetBackPlane():0);
 return result;
}
