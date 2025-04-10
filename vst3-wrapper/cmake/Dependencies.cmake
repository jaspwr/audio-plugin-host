CPMAddPackage(
    NAME VST
    VERSION  3.7.7
    URL "https://download.steinberg.net/sdk_downloads/vst-sdk_3.7.7_build-19_2022-12-12.zip"
)

add_library(VST_SDK
    ${VST_SOURCE_DIR}/vst3sdk/public.sdk/source/vst/hosting/connectionproxy.cpp
    ${VST_SOURCE_DIR}/vst3sdk/public.sdk/source/vst/hosting/eventlist.cpp
    ${VST_SOURCE_DIR}/vst3sdk/public.sdk/source/vst/hosting/hostclasses.cpp
    ${VST_SOURCE_DIR}/vst3sdk/public.sdk/source/vst/hosting/module.cpp
    ${VST_SOURCE_DIR}/vst3sdk/public.sdk/source/vst/hosting/module_win32.cpp
    ${VST_SOURCE_DIR}/vst3sdk/public.sdk/source/vst/hosting/parameterchanges.cpp
    ${VST_SOURCE_DIR}/vst3sdk/public.sdk/source/vst/hosting/pluginterfacesupport.cpp
    ${VST_SOURCE_DIR}/vst3sdk/public.sdk/source/vst/hosting/plugprovider.cpp
    ${VST_SOURCE_DIR}/vst3sdk/public.sdk/source/vst/hosting/processdata.cpp
    ${VST_SOURCE_DIR}/vst3sdk/public.sdk/source/vst/utility/stringconvert.cpp
    ${VST_SOURCE_DIR}/vst3sdk/public.sdk/source/vst/vstinitiids.cpp
    ${VST_SOURCE_DIR}/vst3sdk/public.sdk/source/common/threadchecker_win32.cpp

    ${VST_SOURCE_DIR}/vst3sdk/pluginterfaces/base/conststringtable.cpp
    ${VST_SOURCE_DIR}/vst3sdk/pluginterfaces/base/coreiids.cpp
    ${VST_SOURCE_DIR}/vst3sdk/pluginterfaces/base/funknown.cpp
    ${VST_SOURCE_DIR}/vst3sdk/base/source/fobject.cpp
    ${VST_SOURCE_DIR}/vst3sdk/base/source/fdebug.cpp
    ${VST_SOURCE_DIR}/vst3sdk/base/source/updatehandler.cpp
    ${VST_SOURCE_DIR}/vst3sdk/base/thread/source/flock.cpp
)

target_include_directories(VST_SDK
    PUBLIC "${VST_SOURCE_DIR}/vst3sdk/"
)

target_compile_definitions(VST_SDK
    PUBLIC
        "-DRELEASE"
)

CPMAddPackage(
    NAME concurrentqueue
    GITHUB_REPOSITORY cameron314/concurrentqueue
    GIT_TAG v1.0.3
    DOWNLOAD_ONLY True
)

if(concurrentqueue_ADDED)
    add_library(concurrentqueue INTERFACE IMPORTED)
    target_include_directories(concurrentqueue INTERFACE "${concurrentqueue_SOURCE_DIR}")
endif()
