#include <emscripten.h>
#include <dlfcn.h>
#include <BML/StaticMod.h>

// Side modules live until page teardown. BML owns their IMod instances through
// the same factory/destructor registry as source-integrated native plugins.
extern "C" EMSCRIPTEN_KEEPALIVE int BallanceWebRegisterMod(
    const char *id, const char *entry, const char *exit, const char *dataPath) {
    auto create = reinterpret_cast<IMod *(*)(IBML *)>(dlsym(RTLD_DEFAULT, entry));
    auto destroy = reinterpret_cast<void (*)(IMod *)>(dlsym(RTLD_DEFAULT, exit));
    return BML_RegisterStaticModFactory({id, create, destroy, dataPath}) ? 1 : 0;
}
extern "C" EMSCRIPTEN_KEEPALIVE void *BallanceWebModSymbol(const char *name) {
    return dlsym(RTLD_DEFAULT, name);
}
