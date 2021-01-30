#include "include/flutter_mpv/flutter_mpv_plugin.h"

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>

#include "flutter_video_renderer.hpp"

namespace
{

  class FlutterMpvPlugin : public flutter::Plugin
  {
  public:
    static void RegisterWithRegistrar(flutter::PluginRegistrarWindows *registrar);

    FlutterMpvPlugin(flutter::PluginRegistrarWindows *registrar,
                     std::unique_ptr<flutter::MethodChannel<flutter::EncodableValue>> channel);

    virtual ~FlutterMpvPlugin();

  private:
    // Called when a method is called on this plugin's channel from Dart.
    void HandleMethodCall(
        const flutter::MethodCall<flutter::EncodableValue> &method_call,
        std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);

    std::unique_ptr<flutter::MethodChannel<flutter::EncodableValue>> channel_;
    flutter::BinaryMessenger *messenger_;
    flutter::TextureRegistrar *textures_;
  };

  // static
  void FlutterMpvPlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarWindows *registrar)
  {
    auto channel =
        std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
            registrar->messenger(), "flutter_mpv",
            &flutter::StandardMethodCodec::GetInstance());

    auto *channel_pointer = channel.get();

    // Uses new instead of make_unique due to private constructor.
    std::unique_ptr<FlutterMpvPlugin> plugin(
        new FlutterMpvPlugin(registrar, std::move(channel)));

    channel_pointer->SetMethodCallHandler(
        [plugin_pointer = plugin.get()](const auto &call, auto result) {
          plugin_pointer->HandleMethodCall(call, std::move(result));
        });

    registrar->AddPlugin(std::move(plugin));
  }

  FlutterMpvPlugin::FlutterMpvPlugin(
      flutter::PluginRegistrarWindows *registrar,
      std::unique_ptr<flutter::MethodChannel<flutter::EncodableValue>> channel) : channel_(std::move(channel)),
                                                                                  messenger_(registrar->messenger()),
                                                                                  textures_(registrar->texture_registrar())
  {
  }

  FlutterMpvPlugin::~FlutterMpvPlugin() {}

  void FlutterMpvPlugin::HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue> &method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result)
  {
    if (method_call.method_name().compare("createTexture") == 0)
    {
      auto renderer = new flutterMpv::FlutterVideoRenderer(textures_);
      result->Success(flutter::EncodableValue((int64_t)renderer));
    }
    else if (method_call.method_name().compare("getTextureId") == 0)
    {
      auto renderer = (flutterMpv::FlutterVideoRenderer *)*std::get_if<int64_t>(method_call.arguments());
      result->Success(flutter::EncodableValue((int64_t)renderer->texture_id()));
    }
    else if (method_call.method_name().compare("closeTexture") == 0)
    {
      auto renderer = (flutterMpv::FlutterVideoRenderer *)*std::get_if<int64_t>(method_call.arguments());
      delete renderer;
      result->Success();
    }
    else if (method_call.method_name().compare("play") == 0)
    {
      auto renderer = (flutterMpv::FlutterVideoRenderer *)*std::get_if<int64_t>(method_call.arguments());
      renderer->play();
      result->Success();
    }
    else
    {
      result->NotImplemented();
    }
  }

} // namespace

void FlutterMpvPluginRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar)
{
  FlutterMpvPlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarManager::GetInstance()
          ->GetRegistrar<flutter::PluginRegistrarWindows>(registrar));
}
