#pragma once
#include "se_script_component.hpp"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace se
{
    using ScriptFactory = std::function<std::unique_ptr<ScriptComponent>()>;

    inline std::unordered_map<std::string, ScriptFactory>& getScriptRegistry() {
        static std::unordered_map<std::string, ScriptFactory> registry;
        return registry;
    }


    inline std::unique_ptr<ScriptComponent> createScript(const std::string& scriptName) {
        auto& registry = se::getScriptRegistry();
        auto it = registry.find(scriptName);
        if (it != registry.end()) {
            return it->second();
        }
        return nullptr;
    }

    template<typename T>
    bool registerScript(const std::string& name) {
        getScriptRegistry()[name] = []() -> std::unique_ptr<ScriptComponent> {
            return std::make_unique<T>();
            };
        return true;
    }
}
