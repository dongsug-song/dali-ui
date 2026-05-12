/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file  web-view-input-screenshot-test.cpp
 * @brief Tests input event APIs (mouse, key, touch forwarding) and screenshot APIs.
 *
 * Key bindings
 *   M        Toggle SetMouseEventsEnabled / GetMouseEventsEnabled
 *   K        Toggle SetKeyEventsEnabled / GetKeyEventsEnabled
 *   W        FeedMouseWheel  (simulate down-scroll 3 steps at center)
 *   G        GetScreenshot  (sync)  — logs ImageView URL
 *   A        GetScreenshotAsynchronously  — logs URL on callback
 *   D        FeedKeyEvent  (manually feed Tab key to web engine)
 *   ESC/BACK Quit
 *
 * Signals verified:
 *   KeyEventSignal   — logs key name; returns false (does NOT consume)
 *   TouchEventSignal — logs touch count; returns false (does NOT consume)
 */

#include <dali-ui-foundation/dali-ui-foundation.h>
#include <dali-ui-foundation/public-api/web-view.h>
#include <dali/devel-api/events/key-event-devel.h>
#include <dali/integration-api/debug.h>
#include <cstdio>

using namespace Dali;
using namespace Dali::Ui;

namespace
{
const float INFO_HEIGHT = 32.0f;
const float HINT_HEIGHT = 28.0f;
const float FONT_HINT   = 10.0f;
const float FONT_INFO   = 11.0f;

const UiColor BG_DARK(0x12, 0x12, 0x1F, 0xFF);
const UiColor BG_INFO(0x1E, 0x1E, 0x30, 0xFF);
const UiColor FG_WHITE(0xFF, 0xFF, 0xFF, 0xFF);
const UiColor FG_GRAY(0x88, 0x88, 0xAA, 0xFF);

const char* HINT_TEXT =
  "M:ToggleMouse  K:ToggleKey  W:FeedWheel  G:Screenshot(sync)  A:Screenshot(async)  D:FeedKeyEvent(Tab)";

// Interactive test page with a focused input and mouse-event indicator
const char* TEST_HTML =
  "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Input/Screenshot Test</title>"
  "<style>"
  "body{background:#1a1a2e;color:#eee;font-family:sans-serif;padding:24px}"
  "h2{color:#89b4fa}"
  "#log{background:#0d0d1a;padding:12px;border-radius:6px;font-size:12px;"
  "min-height:100px;white-space:pre-wrap;overflow-y:auto;max-height:200px}"
  "input[type=text]{background:#2a2a3e;color:#eee;border:1px solid #555;"
  "padding:6px;border-radius:4px;width:80%;font-size:14px;outline:none}"
  "</style></head>"
  "<body>"
  "<h2>Input &amp; Screenshot Test</h2>"
  "<p>Type in the field below (key events must be enabled):</p>"
  "<input type='text' id='inp' placeholder='key events will appear here' />"
  "<p>Mouse/touch interactions update the log:</p>"
  "<div id='log'>Waiting for events...</div>"
  "<script>"
  "var log=document.getElementById('log');"
  "function appendLog(msg){"
  "  log.textContent+=msg+'\\n';"
  "  log.scrollTop=log.scrollHeight;"
  "}"
  "document.addEventListener('click',function(e){"
  "  appendLog('click at ('+e.clientX+','+e.clientY+')');"
  "});"
  "document.addEventListener('keydown',function(e){"
  "  appendLog('keydown: '+e.key);"
  "});"
  "document.addEventListener('mousemove',function(e){"
  "  // only log occasional moves"
  "  if(e.movementX*e.movementX+e.movementY*e.movementY>900)"
  "    appendLog('mousemove: ('+e.clientX+','+e.clientY+')');"
  "});"
  "document.addEventListener('wheel',function(e){"
  "  appendLog('wheel deltaY='+e.deltaY);"
  "});"
  "document.getElementById('inp').focus();"
  "</script>"
  "</body></html>";
} // namespace

class InputScreenshotTestController : public ConnectionTracker
{
public:
  explicit InputScreenshotTestController(Application& application)
  : mApplication(application)
  {
    mApplication.InitSignal().Connect(this, &InputScreenshotTestController::OnInit);
  }

private:
  void OnInit(Application application)
  {
    Window window = application.GetWindow();
    window.SetBackgroundColor(Color::BLACK);
    mWindowSize = window.GetSize();

    mHintLabel = Label::New(HINT_TEXT)
      .SetFontSize(FONT_HINT)
      .SetHorizontalTextAlignment(Text::Alignment::START)
      .SetVerticalTextAlignment(Text::Alignment::CENTER)
      .SetTextColor(FG_GRAY)
      .SetBackgroundColor(BG_DARK)
      .SetRequestedWidth(MATCH_PARENT)
      .SetRequestedHeight(HINT_HEIGHT)
      .SetPadding(Extents(6, 6, 0, 0));

    mInfoLabel = Label::New("mouse=ON  key=ON  signals: listening")
      .SetFontSize(FONT_INFO)
      .SetHorizontalTextAlignment(Text::Alignment::START)
      .SetVerticalTextAlignment(Text::Alignment::CENTER)
      .SetTextColor(FG_WHITE)
      .SetBackgroundColor(BG_INFO)
      .SetRequestedWidth(MATCH_PARENT)
      .SetRequestedHeight(INFO_HEIGHT)
      .SetPadding(Extents(6, 6, 0, 0));

    mWebView = WebView::New()
      .SetRequestedWidth(MATCH_PARENT)
      .SetRequestedHeight(0.0f)
      .SetLayoutParams(StackLayoutParams::New()
                         .SetWeight(1.0f)
                         .SetAlignment(LayoutAlignment::FILL));

    StackLayout root = StackLayout::New(StackOrientation::VERTICAL)
      .SetRequestedWidth(MATCH_PARENT)
      .SetRequestedHeight(MATCH_PARENT)
      .Children({mHintLabel, mInfoLabel, mWebView});

    window.Add(root);

    // Start with mouse and key events enabled (default)
    mMouseEnabled = true;
    mKeyEnabled   = true;
    mWebView.SetMouseEventsEnabled(true);
    mWebView.SetKeyEventsEnabled(true);

    // Connect to WebView's input event signals for observation
    mWebView.KeyEventSignal().Connect(this, &InputScreenshotTestController::OnWebViewKeyEvent);
    mWebView.TouchEventSignal().Connect(this, &InputScreenshotTestController::OnWebViewTouchEvent);

    mWebView.PageLoadFinishedSignal().Connect(this, &InputScreenshotTestController::OnPageLoadFinished);

    window.KeyEventSignal().Connect(this, &InputScreenshotTestController::OnWindowKeyEvent);

    mWebView.LoadHtmlString(Dali::String(TEST_HTML));
    UpdateStateLabel();
    DALI_LOG_RELEASE_INFO("[InputTest] Init complete. mouse=%s key=%s\n",
                          mMouseEnabled ? "ON" : "OFF",
                          mKeyEnabled ? "ON" : "OFF");
  }

  // ---------------------------------------------------------------------------
  // WebView input signals (observation — do NOT consume)
  // ---------------------------------------------------------------------------

  bool OnWebViewKeyEvent(WebView /*view*/, KeyEvent event)
  {
    DALI_LOG_RELEASE_INFO("[InputTest] KeyEventSignal: key=%s state=%d\n",
                          event.GetKeyName().c_str(),
                          static_cast<int>(event.GetState()));
    return false; // do not consume
  }

  bool OnWebViewTouchEvent(WebView /*view*/, TouchEvent touch)
  {
    DALI_LOG_RELEASE_INFO("[InputTest] TouchEventSignal: points=%u\n",
                          touch.GetPointCount());
    return false; // do not consume
  }

  void OnPageLoadFinished(WebView /*view*/, const Dali::String& /*url*/)
  {
    DALI_LOG_RELEASE_INFO("[InputTest] Page load finished. mouse=%s key=%s\n",
                          mMouseEnabled ? "ON" : "OFF",
                          mKeyEnabled ? "ON" : "OFF");
  }

  // ---------------------------------------------------------------------------
  // Screenshot callbacks
  // ---------------------------------------------------------------------------

  void OnScreenshotCaptured(Dali::Ui::ImageView screenshot)
  {
    if(!screenshot)
    {
      SetInfo("Screenshot async: FAILED (null ImageView)");
      DALI_LOG_RELEASE_INFO("[InputTest] GetScreenshotAsynchronously: FAILED\n");
      return;
    }
    Dali::String url = screenshot.GetResourceUrl();
    SetInfo("Screenshot async: OK  url=%s", url.CStr());
    DALI_LOG_RELEASE_INFO("[InputTest] GetScreenshotAsynchronously: OK  url=\"%s\"\n", url.CStr());
  }

  // ---------------------------------------------------------------------------
  // Window key events (test control)
  // ---------------------------------------------------------------------------

  void OnWindowKeyEvent(KeyEvent event)
  {
    if(event.GetState() != KeyEvent::DOWN)
    {
      return;
    }

    const std::string key = event.GetKeyName();

    if(IsKey(event, Dali::DALI_KEY_ESCAPE) || IsKey(event, Dali::DALI_KEY_BACK))
    {
      mApplication.Quit();
    }
    else if(key == "M" || key == "m")
    {
      mMouseEnabled = !mMouseEnabled;
      mWebView.SetMouseEventsEnabled(mMouseEnabled);
      bool actual = mWebView.GetMouseEventsEnabled();
      UpdateStateLabel();
      DALI_LOG_RELEASE_INFO("[InputTest] SetMouseEventsEnabled(%s) -> GetMouseEventsEnabled=%s\n",
                            mMouseEnabled ? "true" : "false",
                            actual ? "true" : "false");
    }
    else if(key == "K" || key == "k")
    {
      mKeyEnabled = !mKeyEnabled;
      mWebView.SetKeyEventsEnabled(mKeyEnabled);
      bool actual = mWebView.GetKeyEventsEnabled();
      UpdateStateLabel();
      DALI_LOG_RELEASE_INFO("[InputTest] SetKeyEventsEnabled(%s) -> GetKeyEventsEnabled=%s\n",
                            mKeyEnabled ? "true" : "false",
                            actual ? "true" : "false");
    }
    else if(key == "W" || key == "w")
    {
      int cx = mWindowSize.GetWidth() / 2;
      int cy = mWindowSize.GetHeight() / 2;
      mWebView.FeedMouseWheel(/*yDirection=*/true, /*step=*/3, cx, cy);
      SetInfo("FeedMouseWheel(y=true, step=3, center=%d,%d)", cx, cy);
      DALI_LOG_RELEASE_INFO("[InputTest] FeedMouseWheel(y=true, step=3, x=%d, y=%d)\n", cx, cy);
    }
    else if(key == "G" || key == "g")
    {
      int w = mWindowSize.GetWidth();
      int h = mWindowSize.GetHeight();
      Dali::Rect<int32_t> area(0, 0, w, h);
      auto screenshot = mWebView.GetScreenshot(area, 1.0f);
      if(screenshot)
      {
        Dali::String url = screenshot.GetResourceUrl();
        SetInfo("Screenshot sync: OK  url=%s", url.CStr());
        DALI_LOG_RELEASE_INFO("[InputTest] GetScreenshot: OK  url=\"%s\"\n", url.CStr());
      }
      else
      {
        SetInfo("Screenshot sync: FAILED (null ImageView)");
        DALI_LOG_RELEASE_INFO("[InputTest] GetScreenshot: FAILED\n");
      }
    }
    else if(key == "A" || key == "a")
    {
      int w = mWindowSize.GetWidth();
      int h = mWindowSize.GetHeight();
      Dali::Rect<int32_t> area(0, 0, w, h);
      bool ok = mWebView.GetScreenshotAsynchronously(
        area,
        1.0f,
        WebView::ScreenshotCapturedCallback::New(this, &InputScreenshotTestController::OnScreenshotCaptured));
      SetInfo("GetScreenshotAsynchronously: %s — waiting...", ok ? "started" : "FAILED");
      DALI_LOG_RELEASE_INFO("[InputTest] GetScreenshotAsynchronously: %s\n", ok ? "started" : "FAILED");
    }
    else if(key == "D" || key == "d")
    {
      // Feed a Tab key event directly to the web engine (regardless of SetKeyEventsEnabled)
      KeyEvent tabEvent = DevelKeyEvent::New(
        Dali::String("Tab"), Dali::String("Tab"), Dali::String("\t"),
        9, 0, 0UL, KeyEvent::DOWN,
        Dali::String(""), Dali::String(""),
        Device::Class::NONE, Device::Subclass::NONE);
      bool consumed = mWebView.FeedKeyEvent(tabEvent);
      SetInfo("FeedKeyEvent(Tab): consumed=%s", consumed ? "true" : "false");
      DALI_LOG_RELEASE_INFO("[InputTest] FeedKeyEvent(Tab): consumed=%s\n",
                            consumed ? "true" : "false");
    }
  }

  // ---------------------------------------------------------------------------
  // Helpers
  // ---------------------------------------------------------------------------

  void UpdateStateLabel()
  {
    SetInfo("mouse=%s  key=%s  WebView signals: listening",
            mMouseEnabled ? "ON" : "OFF",
            mKeyEnabled ? "ON" : "OFF");
  }

  template<typename... Args>
  void SetInfo(const char* fmt, Args... args)
  {
    char buf[256];
    snprintf(buf, sizeof(buf), fmt, args...);
    mInfoLabel.SetText(Dali::String(buf));
  }

private:
  Application&       mApplication;
  Label              mHintLabel;
  Label              mInfoLabel;
  Dali::Ui::WebView  mWebView;
  Window::WindowSize mWindowSize{1920, 1080};

  bool mMouseEnabled{true};
  bool mKeyEnabled{true};
};

int DALI_EXPORT_API main(int argc, char** argv)
{
  Application application = Application::New(&argc, &argv);
  UiConfig::New().Apply();
  InputScreenshotTestController test(application);
  application.MainLoop();
  return 0;
}
