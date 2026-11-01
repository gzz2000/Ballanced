#include <BML/IMod.h>
#include <fstream>
#include <string>
class WebHello final : public IMod {
public:
    explicit WebHello(IBML *bml) : IMod(bml) {}
    const char *GetID() override { return "WebHello"; }
    const char *GetVersion() override { return "1.0.0"; }
    const char *GetName() override { return "Web Hello"; }
    const char *GetAuthor() override { return "BallanceWeb SDK"; }
    const char *GetDescription() override { return "Dynamic WASM mod and resource example"; }
    DECLARE_BML_VERSION;
    void OnLoad() override {
        std::ifstream input(BWMOD_RESOURCE_ROOT "/message.txt");
        std::string message;std::getline(input,message);
        GetLogger()->Info("WEB_HELLO_LOADED resource=%s",message.c_str());
    }
    void OnPreStartMenu() override { GetLogger()->Info("WEB_HELLO_MENU_CALLBACK"); }
    void OnUnload() override { GetLogger()->Info("WEB_HELLO_UNLOADED"); }
};
MOD_EXPORT IMod *BMLEntry(IBML *bml) { return new WebHello(bml); }
MOD_EXPORT void BMLExit(IMod *mod) { delete mod; }
