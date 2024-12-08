#include <node.h>
#include "obs.hpp"
#include <iostream>
#include <string>
#include <sstream>

namespace obs_plugin {

using v8::FunctionCallbackInfo;
using v8::Isolate;
using v8::Local;
using v8::Object;
using v8::String;
using v8::Value;



//---------------------------------------------------------------------
#if defined(__linux__) || defined(__APPLE__)
#include <dlfcn.h>
static std::string getPluginLocation()
{
    Dl_info dl_info;
    if (dladdr(reinterpret_cast<void*>(&obs_startup), &dl_info) != 0) {
        std::string fullPath(dl_info.dli_fname);
        size_t pos = fullPath.find_last_of("\\/");
        std::string folder = (pos != std::string::npos) ? fullPath.substr(0, pos) : fullPath;
        // Account for libobs.framework/Versions/A/
        return folder + "/../../../obs-plugins/";
    }
    return "";
}
#else
std::string getPluginLocation(const std::string& subfolder) {
    return "Unsupported platform";
}
#endif

//---------------------------------------------------------------------
void custom_log_handler(int log_level, const char *message, va_list args, void *param)
{
    std::ostringstream oss;
    oss << "[OBS] ";
    if (log_level == LOG_DEBUG) oss << "DEBUG: ";
    else if (log_level == LOG_INFO) oss << "INFO: ";
    else if (log_level == LOG_WARNING) oss << "WARNING: ";
    else if (log_level == LOG_ERROR) oss << "ERROR: ";

    char buffer[4096];
    vsnprintf(buffer, sizeof(buffer), message, args);
    oss << buffer;

    std::cout << oss.str() << std::endl;
}

//---------------------------------------------------------------------
class ObsContext
{
public:
    ObsContext()
    {
    }

    ~ObsContext()
    {
        clearStream();
    }

    bool init()
    {
        std::string plugins = getPluginLocation();
        printf("Loading plugins from %s\n", plugins.c_str());
        base_set_log_handler(custom_log_handler, nullptr);
        bool res = obs_startup("en-US", plugins.c_str(), nullptr);
        if ( !res ) {
            return false;
        }
        mSignalHandler = obs_get_signal_handler();

# if 0 // This fails for some reason; no plugins being reported as loaded
        obs_load_all_modules();
        struct obs_module_failure_info mfi {nullptr,0};
        obs_load_all_modules2(&mfi);
        printf("%d modules failed to load \n", mfi.count);
        for (int i=0; i<mfi.count; i++ ) {
            printf("Failed to load %s\n", mfi.failed_modules[i]);
        }
        obs_module_failure_info_free(&mfi);
        // Lets see those plugins
        obs_post_load_modules();
#else // This actually works ...

        obs_module_t *module;
        int r=obs_open_module(&module,
                        std::string(plugins+"/obs-outputs.plugin/Contents/MacOS/obs-outputs").c_str(),
                        std::string(plugins+"/obs-outputs.plugin/Contents/Resources").c_str());
        if ( r >= 0 ) {
            r = obs_init_module(module);
            if ( r < 0 ) {
                printf("Failed to init outputs plugin, err=%d\n",r);
                return false;
            }
        } else {
            printf("Failed to load outputs plugin, err=%d\n",r);
            return false;
        }
#endif

        // Lets see those plugins
        obs_log_loaded_modules();
        return true;
    }

    bool configureTwitchStream(const std::string& stream_key)
    {
        obs_data_t *settings = obs_data_create();
        if ( !settings ) {
            return false;
        }

        // Set service type to Twitch
        obs_data_set_string(settings, "service", "Twitch");
        obs_data_set_string(settings, "server", "rtmp://live.twitch.tv/app");
        obs_data_set_string(settings, "key", stream_key.c_str());

        mOutput = obs_output_create("rtmp_output", "RTMP Stream", settings, nullptr);
        bool res = (mOutput != nullptr);
        obs_data_release(settings);
        return res;
    }

    bool startStream()
    {
        if ( mStreamRunning ) {
            return true;
        }
        mStreamRunning = obs_output_start(mOutput);
        return mStreamRunning;
    }

    void stopStream()
    {
        if ( mStreamRunning ) {
            obs_output_stop(mOutput);
            mStreamRunning = false;
        }
    }

    void clearStream()
    {
        stopStream();
        obs_output_release(mOutput);
    }

private:
    signal_handler_t*    mSignalHandler = nullptr;
    obs_output_t*        mOutput = nullptr;
    bool                 mStreamRunning = false;
};


//---------------------------------------------------------------------
std::shared_ptr<ObsContext>     gObsContext;


//---------------------------------------------------------------------
static void DumpResult(const FunctionCallbackInfo<Value>& args, const std::string& result)
{
    Isolate* isolate = args.GetIsolate();
    printf("%s\n", result.c_str());
    args.GetReturnValue().Set(String::NewFromUtf8(
            isolate, result.c_str()).ToLocalChecked());
}

//---------------------------------------------------------------------
static void InitOBS(const FunctionCallbackInfo<Value>& args)
{
    std::string result;

    if ( gObsContext ) {
        result = "OBS already initialized";
    } else {
        gObsContext = std::make_shared<ObsContext>();
        if ( !gObsContext->init() ) {
            gObsContext = nullptr;
            result = "Failed to init OBS";
        }
        if ( !gObsContext->configureTwitchStream("CHANGEME") ) {
            result = "Failed to configure Twitch stream";
        } else {
            result = "Successfully initialized OBS";
        }
    }

    DumpResult(args, result);
}

//---------------------------------------------------------------------
static void StartOBS(const FunctionCallbackInfo<Value>& args)
{
    std::string result = "OBS context isn't initialized";
    if ( gObsContext ) {
        printf("Attempting to start the stream\n");
        result = gObsContext->startStream() ? "stream started" : "stream failed to start";
    }
    DumpResult(args, result);
}

//---------------------------------------------------------------------
static void StopOBS(const FunctionCallbackInfo<Value>& args)
{
    std::string result = "OBS context isn't initialized";
    if ( gObsContext ) {
        gObsContext->stopStream();
        result = "stream stopped";
    }
    DumpResult(args, result);
}

//---------------------------------------------------------------------
static void CleanupOBS(const FunctionCallbackInfo<Value>& args)
{
    gObsContext = nullptr;
    DumpResult(args, "OBS context cleared");
}

//---------------------------------------------------------------------
void Initialize(Local<Object> exports)
{
    NODE_SET_METHOD(exports, "init_obs",     InitOBS);
    NODE_SET_METHOD(exports, "start_obs",    StartOBS);
    NODE_SET_METHOD(exports, "stop_obs",     StopOBS);
    NODE_SET_METHOD(exports, "cleanup_obs",  CleanupOBS);
}

//---------------------------------------------------------------------
NODE_MODULE(NODE_GYP_MODULE_NAME, Initialize)

}  // namespace demo copy
