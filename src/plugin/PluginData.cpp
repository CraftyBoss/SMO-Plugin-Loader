#include <plugin/PluginData.h>

void PluginData::runPluginMain(LoaderCtx& ctx) {
    PluginMain func = nullptr;
    if(getPluginFunc(func, "plugin_main")) {
        func(ctx);
    }
}
