cmake_minimum_required(VERSION 3.18)
project(oe_image_processing LANGUAGES CXX CSharp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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
    Session/InpaintingSession.cs
    Session/ProcessingSession.cs
    Session/PipelineDemoSession.cs
    Utils/ImageConversion.cs
    UI/FormMain.resx
    UI/FormPipeLine.resx
    UI/FormInpaint.resx
    UI/FormProcess.resx
    UI/OutputControl.resx
)

set(SRC_FRONT_GUI
    Program.cs
    UI/FormMain.cs
    UI/FormMain.Designer.cs
    UI/FormPipeline.cs
    UI/FormPipeline.Designer.cs
    UI/FormInpaint.cs
    UI/FormInpaint.Designer.cs
    UI/FormProcess.cs
    UI/FormProcess.Designer.cs
    UI/OutputControl.cs
    UI/OutputControl.Designer.cs
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
