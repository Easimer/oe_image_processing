cmake_minimum_required(VERSION 3.18)
project(oe_image_processing LANGUAGES CXX CSharp)

find_package(OpenCV REQUIRED)

include(CSharpUtilities)

set(SRC_LIB
    oeip.def
    public_api.h
    public_api.cpp

    oeip.cpp
    oeip.h
)

set(SRC_FRONT
    OEIP.cs
)

set(SRC_FRONT_GUI
    Program.cs
)

add_library(core SHARED ${SRC_LIB})
target_link_libraries(core PRIVATE ${OpenCV_LIBS})
target_include_directories(core PRIVATE ${OpenCV_INCLUDE_DIRS})

add_executable(front ${SRC_FRONT} ${SRC_FRONT_GUI})

set_property(TARGET front PROPERTY VS_DOTNET_REFERENCES
    "Microsoft.CSharp"
    "PresentationCore"
    "PresentationFramework"
    "System"
    "System.Core"
    "System.Data"
    "System.Data.DataSetExtensions"
    "System.Net.Http"
    "System.Xaml"
    "System.Xml"
    "System.Xml.Linq"
    "WindowsBase"
)

target_compile_options(front PRIVATE -langversion:latest /platform:x64)

set_property(TARGET front PROPERTY WIN32_EXECUTABLE TRUE)
set_property(TARGET front PROPERTY VS_DOTNET_TARGET_FRAMEWORK_VERSION "v4.7.2")

