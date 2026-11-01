// Negative loader fixture. Never install in a real user profile.
#include <BML/IMod.h>
#include <cstdio>

extern "C" void BallanceWeb_IntentionallyAbsentHostFunction_E3970();

struct ConstructorProbe {
    ConstructorProbe() {
        // A preflight failure must happen before this observable side effect.
        if (FILE* file = std::fopen("/user/missing-import-constructor-ran.txt", "wb")) {
            std::fputs("constructor executed\n", file);
            std::fclose(file);
        }
        BallanceWeb_IntentionallyAbsentHostFunction_E3970();
    }
};
static ConstructorProbe constructorProbe;

class MissingImportProbe final : public IMod {
public:
    explicit MissingImportProbe(IBML* bml) : IMod(bml) {}
    const char* GetID() override { return "MissingImportProbe"; }
    const char* GetVersion() override { return "0.1.0"; }
    const char* GetName() override { return "Missing Import Negative Test"; }
    const char* GetAuthor() override { return "BallanceWeb SDK tests"; }
    const char* GetDescription() override { return "Deliberately requires an absent host function"; }
    DECLARE_BML_VERSION;
    void OnLoad() override { GetLogger()->Error("MISSING_IMPORT_ONLOAD_EXECUTED"); }
};
MOD_EXPORT IMod* BMLEntry(IBML* bml) { return new MissingImportProbe(bml); }
MOD_EXPORT void BMLExit(IMod* mod) { delete mod; }
