outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
PhysicaLocation = "%{wks.location}/LabCore"
externalincludename = "LabCore"

include "Dependencies.lua"

workspace "RPLab"
    architecture "x64"
    configurations
    {
        "Debug",
        "Release",
        "Dist"
    }
    startproject "RPLab"


    
group "RPLab"
    include "RPLab"
group ""

group "LabCore"
include "LabCore/buildoptions/externalworkspace.lua"
group ""


