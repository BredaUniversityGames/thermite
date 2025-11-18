#pragma once

#include <string>

namespace thermite
{
    #if THERMITE_EDITOR
    #   pragma message("1")
    #else
    #   pragma message("0")
    #endif

    #if THERMITE_DEBUG
    #   pragma message("1")
    #else
    #   pragma message("0")
    #endif

    class Application
    {
    public:
        explicit Application(std::string name);

        void Run();
        const std::string& GetName() const;

    private:
        std::string m_name;
    };
}