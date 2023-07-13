#include <plugin/PluginData.h>

bool PluginData::runPluginMain(LoaderCtx& ctx) {
    PluginMain func = nullptr;
    if(getPluginFunc(func, "plugin_main")) {
        return func(ctx);
    }
    return false;
}
