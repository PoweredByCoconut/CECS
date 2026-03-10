#include "renderer.h"
#include "types.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

const u32 WIDTH = 800;
const u32 HEIGHT = 600;

const char* const validation_layers[] = {
    "VK_LAYER_KHRONOS_validation",
};

#ifdef NDEBUG
const bool enable_validation_layers = false;
#else
const bool enable_validation_layers = true;
#endif

VkBool32 debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
    printf("DEBUG: %s\n", pCallbackData->pMessage);
    return VK_FALSE;
}

struct Renderer {
    GLFWwindow* window;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_messenger;
    VkSurfaceKHR surface;
    VkPhysicalDevice physical_device;
    VkDevice device;
    VkQueue graphics_queue;
    VkQueue present_queue;
    VkSwapchainKHR swap_chain;
    u32 swap_chain_image_count;
    VkImage* swap_chain_images;
    VkFormat swap_chain_image_format;
    VkExtent2D swap_chain_extent;
    u32 swap_chain_image_view_count;
    VkImageView* swap_chain_image_views;
};

void init_window(Renderer* renderer) {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    renderer->window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Test", NULL, NULL);
}

void create_instance(Renderer* renderer) {
    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Hello Triangle",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_4,
    };

    u32 glfw_extension_count = 0;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    u32 extension_count = enable_validation_layers ? glfw_extension_count + 1 : glfw_extension_count;
    const char** extensions = calloc(extension_count, sizeof(const char*));

    if (enable_validation_layers) {
        memcpy(extensions, glfw_extensions, glfw_extension_count * sizeof(const char*));
        extensions[extension_count - 1] = "VK_EXT_debug_utils";
    }

    printf("required extensions\n");
    for (int i = 0; i < extension_count; i++) {
        printf("   -\t%s\n", extensions[i]);
    }

    u32 available_extension_count = 0;
    vkEnumerateInstanceExtensionProperties(NULL, &available_extension_count, NULL);

    VkExtensionProperties available_extensions[available_extension_count];
    if (vkEnumerateInstanceExtensionProperties(NULL, &available_extension_count, available_extensions) != VK_SUCCESS) {
        fprintf(stderr, "Could not retrieve vulkan extensions\n");
        exit(-1);
    }

    for (u32 i = 0; i < glfw_extension_count; i++) {
        bool has_extension = false;
        for (u32 j = 0; j < available_extension_count; j++) {
            if (!strncmp(glfw_extensions[i], available_extensions[j].extensionName, 256)) {
                has_extension = true;
                break;
            }
        }

        if (!has_extension) {
            fprintf(stderr, "Vulkan context does not provide all required extensions\n");
            exit(-1);
        }
    }

    u32 layer_count = 0;
    vkEnumerateInstanceLayerProperties(&layer_count, NULL);
    VkLayerProperties layers[layer_count];
    vkEnumerateInstanceLayerProperties(&layer_count, layers);

    u32 validation_layer_count = sizeof(validation_layers) / sizeof(validation_layers[0]);

    u32 enabled_layer_count = 0;
    const char* enabled_layers[validation_layer_count];

    for (u32 i = 0; i < validation_layer_count; i++) {
        bool has_layer = false;

        for (u32 j = 0; j < layer_count; j++) {
            if (!strncmp(validation_layers[i], layers[j].layerName, 256)) {
                has_layer = true;
                break;
            }
        }

        if (has_layer) {
            enabled_layers[enabled_layer_count++] = validation_layers[i];
        }
        else {
            fprintf(stderr, "Validation layer %s not found\n", validation_layers[i]);
            exit(-1);
        }
    }

    VkInstanceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledExtensionCount = extension_count,
        .ppEnabledExtensionNames = extensions,
        .enabledLayerCount = enabled_layer_count,
        .ppEnabledLayerNames = enabled_layers,
    };

    if (vkCreateInstance(&create_info, NULL, &renderer->instance) != VK_SUCCESS) {
        fprintf(stderr, "Could not create vulkan instance\n");
        return;
    }
}

VkResult vkCreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pMessenger) {
    PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

    if (func) {
        return func(instance, pCreateInfo, pAllocator, pMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void setup_debug_messenger(Renderer* renderer) {
    if (!enable_validation_layers) {
        return;
    }

    VkDebugUtilsMessageSeverityFlagsEXT severity_flags = 
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

    VkDebugUtilsMessageTypeFlagsEXT message_type_flags = 
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;

    VkDebugUtilsMessengerCreateInfoEXT create_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = severity_flags,
        .messageType = message_type_flags,
        .pfnUserCallback = &debug_callback,
    };

    vkCreateDebugUtilsMessengerEXT(renderer->instance, &create_info, NULL, &renderer->debug_messenger);
}

void create_surface(Renderer* renderer) {
    if (glfwCreateWindowSurface(renderer->instance, renderer->window, NULL, &renderer->surface) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create window surface\n");
        exit(-1);
    }
}

void pick_physical_device(Renderer* renderer) {
    u32 device_count = 0;
    vkEnumeratePhysicalDevices(renderer->instance, &device_count, NULL);

    if (device_count == 0) {
        fprintf(stderr, "Could not find GPU with Vulkan support\n");
        exit(-1);
    }

    VkPhysicalDevice devices[device_count];
    vkEnumeratePhysicalDevices(renderer->instance, &device_count, devices);

    u32 suitable_device_count = 0;
    VkPhysicalDevice suitable_devices[device_count];

    for (int i = 0; i < device_count; i++) {
        VkPhysicalDeviceProperties device_properties;
        vkGetPhysicalDeviceProperties(devices[i], &device_properties);

        u32 queue_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &queue_count, NULL);
        VkQueueFamilyProperties queue_properties[queue_count];
        vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &queue_count, queue_properties);

        bool suitable = device_properties.apiVersion >= VK_API_VERSION_1_3;

        bool has_graphics_bit = false;
        for (int i = 0; i < queue_count; i++) {
            if (queue_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                has_graphics_bit = true;
                break;
            }
        }

        suitable = suitable && has_graphics_bit;

        if (suitable) {
            suitable_devices[suitable_device_count++] = devices[i];
        }
    }

    u32 selection = 0;

    if (suitable_device_count == 0) {
        fprintf(stderr, "No suitable GPU found\n");
        exit(-1);
    }
    else if (suitable_device_count > 1) {
        printf("Enter number [0-%d] to select GPU\n", suitable_device_count - 1);
        for (int i = 0; i < suitable_device_count; i++) {
            VkPhysicalDeviceProperties device_properties;
            vkGetPhysicalDeviceProperties(suitable_devices[i], &device_properties);
            printf("  %d\t%s\n", i, device_properties.deviceName);
        }
        u32 selection = 0;
        scanf("%d", &selection);

        if (selection < 0 && selection >= device_count) {
            printf("Invalid selection, going with first\n");
            selection = 0;
        }
    }
    printf("selected card %d\n", selection);

    renderer->physical_device = devices[selection];
}

VkSurfaceFormatKHR choose_swap_surface_format(u32 available_format_count, VkSurfaceFormatKHR* available_formats) {
    for (int i = 0; i < available_format_count; i++) {
        if (available_formats[i].format == VK_FORMAT_B8G8R8_SRGB && available_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return available_formats[i];
        }
    }

    return available_formats[0];
}

VkPresentModeKHR choose_swap_present_mode(u32 available_present_mode_count, VkPresentModeKHR* available_present_modes) {
    u32 has_relaxed = false;

    for (int i = 0; i < available_present_mode_count; i++) {
        if (available_present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            return available_present_modes[i];
        }
        if (available_present_modes[i] == VK_PRESENT_MODE_FIFO_RELAXED_KHR) {
            has_relaxed = true;
        }
    }

    if (has_relaxed) {
        return VK_PRESENT_MODE_FIFO_RELAXED_KHR;
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

u32 clamp(u32 val, u32 min, u32 max) {
    if (val < min) {
        return min;
    }
    if (val > max) {
        return max;
    }
    return val;
}

u32 max(u32 val1, u32 val2) {
    if (val1 > val2) {
        return val1;
    }
    return val2;
}

VkExtent2D choose_swap_extent(Renderer* renderer, VkSurfaceCapabilitiesKHR* capabilities) {
    if (capabilities->currentExtent.width != UINT32_MAX) {
        return capabilities->currentExtent;
    }
    int width, height;
    glfwGetFramebufferSize(renderer->window, &width, &height);

    return (VkExtent2D) {
        .width = clamp(width, capabilities->minImageExtent.width, capabilities->maxImageExtent.width),
        .height = clamp(height, capabilities->minImageExtent.height, capabilities->maxImageExtent.height)
    };
}

void create_logical_device(Renderer* renderer) {
    u32 properties_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(renderer->physical_device, &properties_count, NULL);
    VkQueueFamilyProperties queue_family_properties[properties_count];
    vkGetPhysicalDeviceQueueFamilyProperties(renderer->physical_device, &properties_count, queue_family_properties);

    u32 graphics_index;
    for (graphics_index = 0;
            graphics_index < properties_count &&
            !(queue_family_properties[graphics_index].queueFlags & VK_QUEUE_GRAPHICS_BIT);
        graphics_index++);

    VkBool32 present_support;
    vkGetPhysicalDeviceSurfaceSupportKHR(renderer->physical_device, graphics_index, renderer->surface, &present_support);

    u32 present_index = graphics_index;
    if (present_support != VK_TRUE) {
        bool found_dub_queue = false;
        for (u32 i = 0; i < properties_count; i++) {
            vkGetPhysicalDeviceSurfaceSupportKHR(renderer->physical_device, i, renderer->surface, &present_support);
            if (queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && present_support) {
                present_index = graphics_index = i;
                found_dub_queue = true;
                break;
            }
        }

        if (!found_dub_queue) {
            for (present_index = 0; present_index < properties_count; present_index++) {
                vkGetPhysicalDeviceSurfaceSupportKHR(renderer->physical_device, present_index, renderer->surface, &present_support);
                if (present_support == VK_TRUE) {
                    break;
                }
            }
        }
    }

    if (graphics_index == properties_count || present_index == properties_count) {
        fprintf(stderr, "No graphics or present queue family found");
        exit(-1);
    }

    const char* device_extensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
    };

    VkPhysicalDeviceExtendedDynamicStateFeaturesEXT extended_dynamic_state_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT,
        .extendedDynamicState = VK_TRUE,
    };

    VkPhysicalDeviceVulkan13Features vulkan13features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .dynamicRendering = VK_TRUE,
        .pNext = &extended_dynamic_state_features,
    };

    VkPhysicalDeviceFeatures2 physical_device_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
    };
    vkGetPhysicalDeviceFeatures2(renderer->physical_device, &physical_device_features);
    physical_device_features.pNext = &vulkan13features;

    float queue_priority = 0.5f;

    VkDeviceQueueCreateInfo device_queue_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = graphics_index,
        .queueCount = 1,
        .pQueuePriorities = &queue_priority,
    };

    VkDeviceCreateInfo device_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &device_queue_create_info,
        .enabledExtensionCount = sizeof(device_extensions) / sizeof(device_extensions[0]),
        .ppEnabledExtensionNames = device_extensions,
        .pNext = &physical_device_features,
    };

    vkCreateDevice(renderer->physical_device, &device_create_info, NULL, &renderer->device);

    vkGetDeviceQueue(renderer->device, graphics_index, 0, &renderer->graphics_queue);
    vkGetDeviceQueue(renderer->device, present_index, 0, &renderer->present_queue);
}

void create_swap_chain(Renderer* renderer) {
    VkSurfaceCapabilitiesKHR surface_capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(renderer->physical_device, renderer->surface, &surface_capabilities);

    u32 available_format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(renderer->physical_device, renderer->surface, &available_format_count, NULL);
    VkSurfaceFormatKHR available_formats[available_format_count];
    vkGetPhysicalDeviceSurfaceFormatsKHR(renderer->physical_device, renderer->surface, &available_format_count, available_formats);

    u32 available_present_mode_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(renderer->physical_device, renderer->surface, &available_present_mode_count, NULL);
    VkPresentModeKHR available_present_modes[available_format_count];
    vkGetPhysicalDeviceSurfacePresentModesKHR(renderer->physical_device, renderer->surface, &available_present_mode_count, available_present_modes);

    VkSurfaceFormatKHR swap_chain_surface_format = choose_swap_surface_format(available_format_count, available_formats);
    VkExtent2D swap_chain_extent = choose_swap_extent(renderer, &surface_capabilities);
    u32 min_image_count = max(3u, surface_capabilities.minImageCount);
    min_image_count = (surface_capabilities.maxImageCount > 0 && min_image_count > surface_capabilities.maxImageCount) ? surface_capabilities.maxImageCount : min_image_count;

    VkSwapchainCreateInfoKHR swap_chain_create_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = renderer->surface,
        .minImageCount = min_image_count,
        .imageFormat = swap_chain_surface_format.format,
        .imageColorSpace = swap_chain_surface_format.colorSpace,
        .imageExtent = swap_chain_extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = surface_capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = choose_swap_present_mode(available_present_mode_count, available_present_modes),
        .clipped = true,
        .oldSwapchain = NULL,
    };

    vkCreateSwapchainKHR(renderer->device, &swap_chain_create_info, NULL, &renderer->swap_chain);
    vkGetSwapchainImagesKHR(renderer->device, renderer->swap_chain, &renderer->swap_chain_image_count, NULL);
    renderer->swap_chain_images = calloc(renderer->swap_chain_image_count, sizeof(VkImage));
    vkGetSwapchainImagesKHR(renderer->device, renderer->swap_chain, &renderer->swap_chain_image_count, renderer->swap_chain_images);

    renderer->swap_chain_image_format = swap_chain_surface_format.format;
    renderer->swap_chain_extent = swap_chain_extent;
}

void create_image_views(Renderer* renderer) {
    VkImageViewCreateInfo image_view_create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = renderer->swap_chain_image_format,
        .subresourceRange = (VkImageSubresourceRange) {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        }
    };

    renderer->swap_chain_image_view_count = renderer->swap_chain_image_count;
    renderer->swap_chain_image_views = realloc(renderer->swap_chain_image_views, sizeof(VkImage) * renderer->swap_chain_image_view_count);

    for (int i = 0; i < renderer->swap_chain_image_count; i++) {
        image_view_create_info.image = renderer->swap_chain_images[i];
        vkCreateImageView(renderer->device, &image_view_create_info, NULL, renderer->swap_chain_image_views + i);
    }
}

void init_vulkan(Renderer* renderer) {
    create_instance(renderer);
    setup_debug_messenger(renderer);
    create_surface(renderer);
    pick_physical_device(renderer);
    create_logical_device(renderer);
    create_swap_chain(renderer);
    create_image_views(renderer);
}

void main_loop(Renderer* renderer) {
    while (!glfwWindowShouldClose(renderer->window)) {
        glfwPollEvents();
        glfwSwapBuffers(renderer->window);
    }
}

void cleanup(Renderer* renderer) {
    glfwDestroyWindow(renderer->window);

    glfwTerminate();

    free(renderer->swap_chain_images);
    free(renderer->swap_chain_image_views);
}

Renderer* renderer_new(void) {
    Renderer* renderer = calloc(sizeof(Renderer), 1);
    if (!renderer) {
        fprintf(stderr, "Failed to allocate renderer\n");
    }
    return renderer;
}

void renderer_run(Renderer* renderer) {
    init_window(renderer);
    init_vulkan(renderer);
    main_loop(renderer);
    cleanup(renderer);
}
