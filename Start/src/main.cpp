// Include //

#include "vk_main.h"

// Entry Point //

int main() 
{
    Vulkan_App vulkan_app;

    try 
    {
        vulkan_app.run();
    } catch (const std::exception& except) 
    {
        std::cerr << except.what() << std::endl;
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}
