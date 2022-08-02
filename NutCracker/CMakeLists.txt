cmake_minimum_required (VERSION 3.21) # C++ 20

find_package (fmt CONFIG REQUIRED)
find_package (glfw3 CONFIG REQUIRED)
find_package (imgui CONFIG REQUIRED)
find_package (glm CONFIG REQUIRED)
find_package (spdlog CONFIG REQUIRED)
find_package (Vulkan REQUIRED)

set (THIS_LIB "_nutcracker_")
file (GLOB HEADER_COLLECTION "NutCracker/**.h" "NutCracker/**.hpp" "NutCracker/**.hxx")

add_library (${THIS_LIB}
    # Headers
        ${HEADER_COLLECTION}
        #"NutCracker/Core/Logging.hxx"
        #"NutCracker/Utils/HelperMacros.hxx"
    # Source - sorted by compilation dependency
        "NutCracker/Base/Event.ixx"
        "NutCracker/Core/LoggingExtn.ixx"
        "NutCracker/Core/KeyCodes.ixx"
        "NutCracker/Core/MouseCodes.ixx"
        "NutCracker/Core/OSUtils.ixx"
        "NutCracker/Utils/Random.ixx"
        "NutCracker/Utils/Assets.ixx"
        "NutCracker/Utils/external_impls/stb.cpp"
        "NutCracker/Utils/Vulkan/DebugMarkers.ixx" 

        "NutCracker/Base/Window.ixx" 
        "NutCracker/Base/AppInstance.ixx"
        "NutCracker/Core/Input.ixx"
        "NutCracker/Core/Event/TypeApplication.ixx"
        "NutCracker/Core/Event/TypeKeyboard.ixx"
        "NutCracker/Core/Event/TypeMouse.ixx"
        "src/Platform/Windows/platformInputTranslation.ixx"  

        "NutCracker/NutCracker_Events.ixx"
        "NutCracker/NutCracker_Base.ixx"
        
        "NutCracker/Example/Skeleton.ixx"
        "src/Platform/Windows/WindowImpl.ixx"

        "src/Platform/Window.cppm"

        "NutCracker/Example/MultipleWindowed.ixx" 

        "NutCracker/NutCracker_Examples.ixx" 
    )

target_include_directories (${THIS_LIB}
    PUBLIC
        "${CMAKE_CURRENT_LIST_DIR}"
        
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/NutCracker"
    )
target_link_libraries (${THIS_LIB}
    PUBLIC
        ${TARGET_SHARED}
        fmt::fmt-header-only
        glfw
        imgui::imgui
        glm::glm
        spdlog::spdlog spdlog::spdlog_header_only
    )

set (TARGET_SHARED ${THIS_LIB} PARENT_SCOPE)