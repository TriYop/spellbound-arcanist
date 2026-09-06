#include "ArcanistPluginAdapter.h"
#include <cstring>

START_NAMESPACE_DISTRHO
Plugin* createPlugin() { return new ArcanistPluginAdapter(); }
END_NAMESPACE_DISTRHO
