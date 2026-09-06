#include "test_runner.h"
#include "../Source/FactoryPresets.h"
#include "../Source/ArcanistParams.h"
#include <cstdint>

int main()
{
    const auto presets = arcanistFactoryPresets();

    CHECK_MSG (presets.size() == 42, "expected all 42 ported factory presets");

    bool foundDxBass1 = false;
    for (const auto& p : presets)
    {
        CHECK_MSG (p.pluginId == "com.spellbound.arcanist", "every preset must stamp Arcanist's plugin id");
        CHECK_MSG (!p.name.empty(), "every preset must have a name");
        CHECK_MSG (!p.parameters.empty(), "every preset must set at least one parameter");

        // Every parameter id in every preset must be a real Arcanist parameter.
        for (const auto& pv : p.parameters)
        {
            bool known = false;
            for (uint32_t i = 0; i < kArcanistParamCount; ++i)
                if (pv.id == kArcanistParamSpecs[i].id) { known = true; break; }
            CHECK_MSG (known, ("unknown parameter id in preset " + p.name + ": " + pv.id).c_str());
        }

        if (p.name == "DX Bass 1") foundDxBass1 = true;
    }
    CHECK_MSG (foundDxBass1, "DX Bass 1 must survive the conversion");

    // No duplicate names.
    for (size_t i = 0; i < presets.size(); ++i)
        for (size_t j = i + 1; j < presets.size(); ++j)
            CHECK_MSG (presets[i].name != presets[j].name, "duplicate preset name");

    TEST_SUMMARY();
    return 0;
}
