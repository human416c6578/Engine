#pragma once
#include "engine/scripting/se_script_component.hpp" // The inline registration template checks and constructs derived scripts.

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace se
{
    using ScriptFactory = std::function<std::unique_ptr<ScriptComponent>()>;

    class ScriptManager
    {
    public:
        ScriptManager() = default;
        ScriptManager(const ScriptManager&) = delete;
        ScriptManager& operator=(const ScriptManager&) = delete;
        ScriptManager(ScriptManager&&) = delete;
        ScriptManager& operator=(ScriptManager&&) = delete;

        template<typename T>
        void registerScript(const std::string& name)
        {
            registry[name] = []() -> std::unique_ptr<ScriptComponent> {
                return std::make_unique<T>();
            };
        }

        std::unique_ptr<ScriptComponent> createScript(const std::string& scriptName) const
        {
            const auto it = registry.find(scriptName);
            return it != registry.end() ? it->second() : nullptr;
        }

        const std::unordered_map<std::string, ScriptFactory>& getRegistry() const
        {
            return registry;
        }

    private:
        std::unordered_map<std::string, ScriptFactory> registry;
    };
}
