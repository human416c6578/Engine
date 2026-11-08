#include "engine/app/app.hpp"

#include <cstdlib>
#include <exception>
#include "engine/core/se_logger.hpp"

int main()
{
    se::Logger logger;
    try
    {
        App app{logger};
        app.run();
    }
    catch (const std::exception &e)
    {
        logger.log(se::LogLevel::Error, "App", "%s", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
