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

    image_inpainting.cpp
    image_inpainting.h
)

set(SRC_FRONT
    App.config
    OEIP.cs
    Inpainting.cs
    ImageConversion.cs
    FormMain.resx
    FormPipeLine.resx
    FormInpaint.resx
    OutputControl.resx
)

set(SRC_FRONT_GUI
    Program.cs
    FormMain.cs
    FormMain.Designer.cs
    FormPipeline.cs
    FormPipeline.Designer.cs
    FormInpaint.cs
    FormInpaint.Designer.cs
    OutputControl.cs
    OutputControl.Designer.cs
    Properties/AssemblyInfo.cs
    Properties/Resources.Designer.cs
    Properties/Resources.resx
    Properties/Settings.Designer.cs
    Properties/Settings.settings
)

csharp_set_windows_forms_properties(${SRC_FRONT_GUI})

add_library(core SHARED ${SRC_LIB})
target_link_libraries(core PRIVATE ${OpenCV_LIBS})
target_include_directories(core PRIVATE ${OpenCV_INCLUDE_DIRS})

add_executable(front ${SRC_FRONT} ${SRC_FRONT_GUI})

set_property(TARGET front PROPERTY VS_DOTNET_REFERENCES
    "Microsoft.CSharp"
    "System"
    "System.Core"
    "System.Data"
    "System.Drawing"
    "System.Windows.Forms"
    "System.Numerics.Vectors"
)

target_compile_options(front PRIVATE -langversion:latest /platform:x64 /unsafe)

set_property(TARGET front PROPERTY WIN32_EXECUTABLE TRUE)
set_property(TARGET front PROPERTY VS_DOTNET_TARGET_FRAMEWORK_VERSION "v4.7.2")

add_dependencies(front core)
