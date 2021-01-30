#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>

#include "libmpv/include/client.h"
#include "libmpv/include/render_gl.h"

#include <mutex>
#include <queue>

namespace flutterMpv
{
  enum MpvTask
  {
    update,
    event,
    none,
  };

  class FlutterVideoRenderer
  {
    flutter::TextureRegistrar *registrar_ = nullptr;
    int64_t texture_id_ = -1;
    std::unique_ptr<flutter::TextureVariant> texture_;
    mpv_handle *mpv;
    std::shared_ptr<FlutterDesktopPixelBuffer> pixel_buffer;
    mutable std::shared_ptr<uint8_t> rgba_buffer;
    mpv_render_context *mpv_rd;
    std::thread thread;
    std::atomic<bool> stoped;
    std::mutex m_lock;
    std::queue<MpvTask> tasks;

  public:
    FlutterVideoRenderer(flutter::TextureRegistrar *registrar);
    ~FlutterVideoRenderer();
    void redraw();
    void play();
    const FlutterDesktopPixelBuffer *CopyPixelBuffer(
        size_t width,
        size_t height);

    int64_t texture_id();
    void commit(MpvTask task);
  };

  void FlutterVideoRenderer::commit(MpvTask task)
  {
    if (stoped.load()) // stop == true ??
      throw std::runtime_error("commit on stopped engine.");
    {                                           // 添加任务到队列
      std::lock_guard<std::mutex> lock{m_lock}; //对当前块的语句加锁  lock_guard 是 mutex 的 stack 封装类，构造的时候 lock()，析构的时候 unlock()
      tasks.emplace(task);
    }
  }

  static void on_mpv_render_update(void *ctx)
  {
    FlutterVideoRenderer *renderer = (FlutterVideoRenderer *)ctx;
    renderer->commit(MpvTask::update);
  }

  static void on_mpv_events(void *ctx)
  {
    FlutterVideoRenderer *renderer = (FlutterVideoRenderer *)ctx;
    renderer->commit(MpvTask::event);
  }

  FlutterVideoRenderer::FlutterVideoRenderer(flutter::TextureRegistrar *registrar) : registrar_(registrar)
  {
    texture_ =
        std::make_unique<flutter::TextureVariant>(flutter::PixelBufferTexture(
            [this](size_t width,
                   size_t height) -> const FlutterDesktopPixelBuffer * {
              return this->CopyPixelBuffer(width, height);
            }));
    texture_id_ = registrar_->RegisterTexture(texture_.get());
    pixel_buffer.reset(new FlutterDesktopPixelBuffer());
    mpv = mpv_create();
    mpv_initialize(mpv);
    // mpv_request_log_messages(mpv, "debug");
    int advCtr{1};
    // mpv_opengl_init_params gl_init_params{get_proc_address, nullptr, nullptr};
    mpv_render_param params[]{
        {MPV_RENDER_PARAM_API_TYPE, MPV_RENDER_API_TYPE_SW},
        {MPV_RENDER_PARAM_ADVANCED_CONTROL, &advCtr},
        {MPV_RENDER_PARAM_INVALID, nullptr}};
    int ret = mpv_render_context_create(&mpv_rd, mpv, params);
    if (ret < 0)
    {
      throw std::exception("failed to initialize mpv GL context");
    }
    mpv_set_wakeup_callback(mpv, on_mpv_events, this);
    mpv_render_context_set_update_callback(mpv_rd, on_mpv_render_update, this);
    thread = std::thread([this] {
      // 循环
      while (!this->stoped)
      {
        // 获取待执行的task
        MpvTask task = MpvTask::none;
        {                                                  // 获取一个待执行的 task
          std::unique_lock<std::mutex> lock{this->m_lock}; // unique_lock 相比 lock_guard 的好处是：可以随时 unlock() 和 lock()
          if (!this->tasks.empty())
          {
            task = this->tasks.front(); // 取一个 task
            this->tasks.pop();
          }
        }
        // 执行task
        switch (task)
        {
        case MpvTask::update:
        {
          uint64_t flags = mpv_render_context_update(mpv_rd);
          if (flags & MPV_RENDER_UPDATE_FRAME)
            redraw();
          break;
        }
        case MpvTask::event:
          while (1)
          {
            mpv_event *mp_event = mpv_wait_event(mpv, 0);
            if (mp_event->event_id == MPV_EVENT_NONE)
              break;
          }
          break;
        }
      }
    });
  }

  int64_t FlutterVideoRenderer::texture_id() { return texture_id_; }

  FlutterVideoRenderer::~FlutterVideoRenderer()
  {
    mpv_render_context_free(mpv_rd);
    mpv_detach_destroy(mpv);
    registrar_->UnregisterTexture(texture_id_);
    stoped.store(true);
    if (thread.joinable())
      thread.join(); // 等待任务结束， 前提：线程一定会执行完
  }

  void FlutterVideoRenderer::redraw()
  {
    registrar_->MarkTextureFrameAvailable(texture_id_);
  }

  const FlutterDesktopPixelBuffer *FlutterVideoRenderer::CopyPixelBuffer(
      size_t width,
      size_t height)
  {
    if (pixel_buffer->width != width || pixel_buffer->height != height)
    {
      size_t buffer_size = (width * height) * (32 >> 3);
      rgba_buffer.reset(new uint8_t[buffer_size]);
      pixel_buffer->width = width;
      pixel_buffer->height = height;
      pixel_buffer->buffer = rgba_buffer.get();
    }
    int size[2] = {pixel_buffer->width, pixel_buffer->height};
    int pitch = pixel_buffer->width * 4;
    mpv_render_param params[] = {
        {MPV_RENDER_PARAM_SW_SIZE, size},
        {MPV_RENDER_PARAM_SW_FORMAT, "rgba"},
        {MPV_RENDER_PARAM_SW_STRIDE, &pitch},
        {MPV_RENDER_PARAM_SW_POINTER, (void *)pixel_buffer->buffer},
        {MPV_RENDER_PARAM_INVALID, nullptr}};
    mpv_render_context_render(mpv_rd, params);
    return pixel_buffer.get();
  };

  void FlutterVideoRenderer::play()
  {
    const char *cmd[] = {"loadfile", "D:\\Downloads\\System\\big_buck_bunny.mp4", NULL};
    mpv_command_async(mpv, 0, cmd);
  }
} // namespace flutterMpv