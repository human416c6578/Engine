#include "engine/app/app.hpp"

#include <cstdlib>
#include <exception>
#include "engine/core/se_logger.hpp"

int main()
{
    try
    {
        App app;
        app.run();
    }
    catch (const std::exception &e)
    {
        se::log::error("App", "%s", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
