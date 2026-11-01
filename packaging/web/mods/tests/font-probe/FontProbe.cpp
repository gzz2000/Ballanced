#include <BML/IMod.h>
#include <VxWindowFunctions.h>
#include <CKStateChunk.h>
#include <cstdio>
#include <cstring>
static IBML *api;
static unsigned pixels[256*96];
static char report[2048];
class FontProbe:public IMod {
public:
 explicit FontProbe(IBML *b):IMod(b){}
 const char*GetID()override{return "FontProbe";}
 const char*GetVersion()override{return "0.1.0";}
 const char*GetName()override{return "Font SDK acceptance probe";}
 const char*GetAuthor()override{return "BallanceWeb tests";}
 const char*GetDescription()override{return "Tests public bitmap/font/serialization APIs";}
 DECLARE_BML_VERSION;
 void OnLoad()override{api=m_BML;}
 void OnUnload()override{api=nullptr;}
};
MOD_EXPORT IMod* BMLEntry(IBML*b){return new FontProbe(b);}
MOD_EXPORT void BMLExit(IMod*m){delete m;}
static VxImageDescEx desc(){VxImageDescEx d;d.Width=256;d.Height=96;d.BytesPerLine=1024;d.BitsPerPixel=32;d.RedMask=0xff0000;d.GreenMask=0xff00;d.BlueMask=0xff;d.AlphaMask=0xff000000;d.Image=(XBYTE*)pixels;return d;}
MOD_EXPORT const char*BML_WebFontPixelsHex(){static char hex[sizeof(pixels)*2+1];const char*d="0123456789abcdef";const auto*p=(const unsigned char*)pixels;for(unsigned i=0;i<sizeof(pixels);i++){hex[i*2]=d[p[i]>>4];hex[i*2+1]=d[p[i]&15];}hex[sizeof(pixels)*2]=0;return hex;}
MOD_EXPORT const char*BML_WebFontDraw(int mode){
 auto d=desc();for(auto&p:pixels)p=0xff17395b;
 if(mode==0){for(int y=0;y<96;y++)for(int x=0;x<256;x++)pixels[y*256+x]=(unsigned(x)<<24)|((unsigned(y*2)&255)<<16)|((unsigned(x+y)&255)<<8)|(unsigned(x*3+y*7)&255);}
 unsigned original[256*96];memcpy(original,pixels,sizeof(pixels));
 auto bitmap=VxCreateBitmap(d);if(!bitmap)return "{\"error\":\"bitmap\"}";
 int drawn=0,height=0,info=0;
 if(mode){auto font=VxCreateFont("Arial",mode>=11?12:18,400,FALSE,FALSE);VXFONTINFO fi;info=VxGetFontInfo(font,fi);height=fi.Height;CKRECT rect{16,8,240,88};
 unsigned align=VXTEXT_LEFT|VXTEXT_TOP;
 if(mode==2)align=VXTEXT_HCENTER|VXTEXT_TOP;if(mode==3)align=VXTEXT_RIGHT|VXTEXT_TOP;
 if(mode==4)align=VXTEXT_LEFT|VXTEXT_VCENTER;if(mode==5)align=VXTEXT_LEFT|VXTEXT_BOTTOM;
 const char*t=mode==6?"↑↓←→":mode==7?"中文测试":"Hello";
 if(mode>=8&&mode<=10)t="Hello\nWorld";
 if(mode==9)align=VXTEXT_LEFT|VXTEXT_VCENTER;
 if(mode==10)align=VXTEXT_LEFT|VXTEXT_BOTTOM;
 if(mode==11)t="Hello\nWorld\nThird";
 if(mode==12)t="Hello\r\nWorld\r\nThird";
 if(mode==13){t="Hello long line\nWorld long line\nThird long line";rect.right=64;rect.bottom=48;}
 drawn=VxDrawBitmapText(bitmap,font,t,&rect,align,0x5b3917,0x27a4e9);VxDeleteFont(font);}
 memset(pixels,0,sizeof(pixels));int copied=VxCopyBitmap(bitmap,d);VxDeleteBitmap(bitmap);
 int changed=0,mismatch=0,minx=256,miny=96,maxx=-1,maxy=-1;
 for(int y=0;y<96;y++)for(int x=0;x<256;x++){unsigned p=pixels[y*256+x];if(p!=original[y*256+x])mismatch++;if(mode&&p!=0xff17395b){changed++;if(x<minx)minx=x;if(x>maxx)maxx=x;if(y<miny)miny=y;if(y>maxy)maxy=y;}}
 snprintf(report,sizeof(report),"{\"mode\":%d,\"copied\":%d,\"drawn\":%d,\"fontInfo\":%d,\"height\":%d,\"mismatch\":%d,\"glyphPixels\":%d,\"bbox\":[%d,%d,%d,%d]}",mode,copied,drawn,info,height,mismatch,changed,minx,miny,maxx,maxy);return report;
}
MOD_EXPORT const char*BML_WebFontSaveCycles(){
 if(!api)return "{}";
 auto*s=static_cast<CKSpriteText*>(api->GetCKContext()->CreateObject(CKCID_SPRITETEXT,"FontProbeSprite"));
 if(!s)return "{\"error\":\"sprite\"}";
 s->Create(256,96,32);s->SetFont("Arial",18,700,TRUE,TRUE);s->SetText("Hello ↑ 中文");
 int sizes[6]={},weights[6]={},errors=0;
 for(int i=0;i<6;i++){auto*c=s->Save(nullptr,0);if(!c){errors++;break;}c->StartRead();
 if(c->SeekIdentifier(CK_STATESAVE_SPRITEFONT)){XString name;c->ReadString(name);sizes[i]=c->ReadInt();weights[i]=c->ReadInt();}else errors++;
 c->StartRead();if(s->Load(c,nullptr)!=CK_OK)errors++;DeleteCKStateChunk(c);}
 snprintf(report,sizeof(report),"{\"errors\":%d,\"sizes\":[%d,%d,%d,%d,%d,%d],\"weights\":[%d,%d,%d,%d,%d,%d]}",errors,sizes[0],sizes[1],sizes[2],sizes[3],sizes[4],sizes[5],weights[0],weights[1],weights[2],weights[3],weights[4],weights[5]);
 api->GetCKContext()->DestroyObject(s);return report;
}
