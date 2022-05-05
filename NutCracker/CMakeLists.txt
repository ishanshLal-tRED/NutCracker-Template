cmake_minimum_required(VERSION 3.21) # C++ 20

find_package(fmt CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(Vulkan REQUIRED)

set(THIS_LIB "_nutcracker_")
file(GLOB HEADER_COLLECTION "NutCracker/**.h" "NutCracker/**.hpp" "NutCracker/**.hxx")

add_library(${THIS_LIB}
    # Headers
        ${HEADER_COLLECTION}
    # Source
        "NutCracker/Base/AppInstance.ixx"
        "NutCracker/Base/Event.ixx"
        "NutCracker/Base/Window.ixx" "src/Platform/Window.cppm" 

        ##"NutCracker/Core/Logging.hxx"
        "NutCracker/Core/LoggingExtn.ixx"
        "NutCracker/Core/KeyCodes.ixx"
        "NutCracker/Core/MouseCodes.ixx"
        "NutCracker/Core/Input.ixx"
        "NutCracker/Core/OSUtils.ixx"

        "NutCracker/Core/Event/TypeApplication.ixx"
        "NutCracker/Core/Event/TypeKeyboard.ixx"
        "NutCracker/Core/Event/TypeMouse.ixx"

        #"NutCracker/Utils/HelperMacros.hxx"
        "NutCracker/Utils/Random.ixx"
        
        "NutCracker/Example/Skeleton.ixx"
        "NutCracker/Example/MultipleWindowed.ixx" 
        "NutCracker/Example/VulkanExample.ixx" 
        "NutCracker/Example/VulkanExample.cppm"

        "src/Platform/Windows/WindowImpl.ixx"

        "NutCracker/NutCracker_Base.ixx"
        "NutCracker/NutCracker_Events.ixx"
        "NutCracker/NutCracker_Examples.ixx" 
     "NutCracker/Utils/Vulkan/DebugMarkers.ixx" "src/Platform/Windows/platformInputTranslation.ixx")

target_include_directories(${THIS_LIB}
    PUBLIC
        "${CMAKE_CURRENT_LIST_DIR}"
        ##"$<INSTALL_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>"
        ##"$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/NutCracker>"
        
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/NutCracker"
    )
target_link_libraries(${THIS_LIB}
    PUBLIC
        ${TARGET_SHARED}
        fmt::fmt-header-only
        glfw
        imgui::imgui
        glm::glm
        spdlog::spdlog spdlog::spdlog_header_only
    )

set(TARGET_SHARED ${THIS_LIB} PARENT_SCOPE)