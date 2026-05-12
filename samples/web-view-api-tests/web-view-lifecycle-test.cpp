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
 * @file  web-view-lifecycle-test.cpp
 * @brief Tests WebView lifecycle, rendering settings, network control, video, and fullscreen APIs.
 *
 * Key bindings
 *   S        Toggle Suspend / Resume
 *   N        Toggle SuspendNetworkLoading / ResumeNetworkLoading
 *   B        Cycle SetDocumentBackgroundColor (transparent → white → dark → red)
 *   T        Toggle SetTilesClearedWhenHidden
 *   M        Cycle SetTileCoverAreaMultiplier (1.0 → 2.0 → 3.0)
 *   C        Toggle SetCursorEnabledByClient
 *   R        ClearAllTilesResources
 *   V        Toggle SetVideoHoleEnabled / GetVideoHoleEnabled
 *   P        CheckVideoPlayingAsynchronously
 *   F        ExitFullscreen
 *   1        Load a video test page (inline HTML5 video)
 *   2        Load a geolocation test page (triggers GeolocationPermissionSignal)
 *   0        Load default test page
 *   ESC/BACK Quit
 *
 * Signals verified:
 *   FrameRenderedSignal         — logs first 3 frames
 *   FullscreenEnteredSignal     — logs entry
 *   FullscreenExitedSignal      — logs exit
 *   WebProcessCrashedSignal     — logs crash
 *   GeolocationPermissionSignal — accepts permission, logs origin
 */

#include <dali-ui-foundation/dali-ui-foundation.h>
#include <dali-ui-foundation/public-api/web-view.h>
#include <dali/integration-api/debug.h>
#include <cstdio>

using namespace Dali;
using namespace Dali::Ui;

namespace
{
const float INFO_HEIGHT = 48.0f;
const float HINT_HEIGHT = 28.0f;
const float FONT_HINT   = 10.0f;
const float FONT_INFO   = 11.0f;

const UiColor BG_DARK(0x12, 0x12, 0x1F, 0xFF);
const UiColor BG_INFO(0x1E, 0x1E, 0x30, 0xFF);
const UiColor FG_WHITE(0xFF, 0xFF, 0xFF, 0xFF);
const UiColor FG_GRAY(0x88, 0x88, 0xAA, 0xFF);

const char* HINT_TEXT =
  "S:Suspend  N:NetSuspend  B:BgColor  T:TilesClear  M:TileMult  "
  "C:Cursor  R:ClearTiles  V:VideoHole  P:VideoPlaying  F:ExitFS  1:Video  2:Geo  0:Default";

// Default test page
const char* DEFAULT_HTML =
  "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Lifecycle Test</title>"
  "<style>"
  "body{background:#1a1a2e;color:#eee;font-family:sans-serif;padding:24px}"
  "h2{color:#89b4fa}#log{background:#0d0d1a;padding:12px;border-radius:6px;"
  "font-size:12px;min-height:80px;white-space:pre-wrap}"
  "</style></head>"
  "<body><h2>WebView Lifecycle &amp; Rendering Test</h2>"
  "<p>Use key bindings to test lifecycle APIs. Check logs in terminal.</p>"
  "<div id='log'>Press keys to test APIs...</div>"
  "</body></html>";

// Video test page with an HTML5 video that supports fullscreen
const char* VIDEO_HTML =
  "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Video Test</title>"
  "<style>body{background:#000;margin:0;display:flex;align-items:center;justify-content:center;height:100vh}"
  "video{max-width:100%;max-height:100%}</style></head>"
  "<body>"
  "<video id='v' controls autoplay loop muted playsinline"
  "  src='https://www.w3schools.com/html/mov_bbb.mp4'>"
  "  Your browser does not support HTML5 video."
  "</video>"
  "<script>"
  "var v=document.getElementById('v');"
  "v.addEventListener('play',function(){console.log('video:playing');});"
  "v.addEventListener('pause',function(){console.log('video:paused');});"
  "</script>"
  "</body></html>";

// Geolocation test page
const char* GEO_HTML =
  "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Geolocation Test</title>"
  "<style>body{background:#1a1a2e;color:#eee;font-family:sans-serif;padding:40px;text-align:center}"
  "h2{color:#89b4fa}#result{margin-top:20px;font-size:14px}</style></head>"
  "<body><h2>Geolocation Permission Test</h2>"
  "<p>This page requests geolocation — GeolocationPermissionSignal will fire.</p>"
  "<div id='result'>Requesting location...</div>"
  "<script>"
  "if(navigator.geolocation){"
  "  navigator.geolocation.getCurrentPosition("
  "    function(p){document.getElementById('result').textContent="
  "      'Location: '+p.coords.latitude.toFixed(4)+', '+p.coords.longitude.toFixed(4);},"
  "    function(e){document.getElementById('result').textContent='Error: '+e.message;}"
  "  );"
  "}else{"
  "  document.getElementById('result').textContent='Geolocation not supported.';"
  "}"
  "</script>"
  "</body></html>";

const Dali::Vector4 BG_COLORS[] = {
  Dali::Vector4(0.0f, 0.0f, 0.0f, 0.0f),   // transparent
  Dali::Vector4(1.0f, 1.0f, 1.0f, 1.0f),   // white
  Dali::Vector4(0.1f, 0.1f, 0.12f, 1.0f),  // dark
  Dali::Vector4(0.3f, 0.0f, 0.0f, 1.0f),   // dark red
};
const char* BG_COLOR_NAMES[] = {"transparent", "white", "dark", "red"};
const int   BG_COLOR_COUNT   = 4;

const float TILE_MULT_CYCLE[] = {1.0f, 2.0f, 3.0f};
const int   TILE_MULT_COUNT   = 3;

const int MAX_FRAME_LOG = 3; // only log first N FrameRendered events
} // namespace

class LifecycleTestController : public ConnectionTracker
{
public:
  explicit LifecycleTestController(Application& application)
  : mApplication(application)
  {
    mApplication.InitSignal().Connect(this, &LifecycleTestController::OnInit);
  }

private:
  void OnInit(Application application)
  {
    Window window = application.GetWindow();
    window.SetBackgroundColor(Color::BLACK);

    mHintLabel = Label::New(HINT_TEXT)
      .SetFontSize(FONT_HINT)
      .SetHorizontalTextAlignment(Text::Alignment::START)
      .SetVerticalTextAlignment(Text::Alignment::CENTER)
      .SetTextColor(FG_GRAY)
      .SetBackgroundColor(BG_DARK)
      .SetRequestedWidth(MATCH_PARENT)
      .SetRequestedHeight(HINT_HEIGHT)
      .SetPadding(Extents(6, 6, 0, 0));

    mInfoLabel = Label::New("active | net=running | videoHole=OFF | tiles=cleared=OFF | mult=1.0x")
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

    mWebView.FrameRenderedSignal().Connect(this, &LifecycleTestController::OnFrameRendered);
    mWebView.FullscreenEnteredSignal().Connect(this, &LifecycleTestController::OnFullscreenEntered);
    mWebView.FullscreenExitedSignal().Connect(this, &LifecycleTestController::OnFullscreenExited);
    mWebView.WebProcessCrashedSignal().Connect(this, &LifecycleTestController::OnWebProcessCrashed);
    mWebView.GeolocationPermissionSignal().Connect(this, &LifecycleTestController::OnGeolocationPermission);
    mWebView.PageLoadFinishedSignal().Connect(this, &LifecycleTestController::OnPageLoadFinished);

    window.KeyEventSignal().Connect(this, &LifecycleTestController::OnKeyEvent);

    mWebView.LoadHtmlString(Dali::String(DEFAULT_HTML));
    UpdateStateLabel();
    DALI_LOG_RELEASE_INFO("[LifecycleTest] Init complete\n");
  }

  // ---------------------------------------------------------------------------
  // Signal handlers
  // ---------------------------------------------------------------------------

  void OnFrameRendered(WebView /*view*/)
  {
    ++mFrameCount;
    if(mFrameCount <= MAX_FRAME_LOG)
    {
      DALI_LOG_RELEASE_INFO("[LifecycleTest] FrameRendered #%u\n", mFrameCount);
    }
  }

  void OnFullscreenEntered(WebView /*view*/)
  {
    mInFullscreen = true;
    SetInfo("FullscreenEntered — press F to exit");
    DALI_LOG_RELEASE_INFO("[LifecycleTest] FullscreenEnteredSignal\n");
  }

  void OnFullscreenExited(WebView /*view*/)
  {
    mInFullscreen = false;
    UpdateStateLabel();
    DALI_LOG_RELEASE_INFO("[LifecycleTest] FullscreenExitedSignal\n");
  }

  void OnWebProcessCrashed(WebView /*view*/)
  {
    SetInfo("!!! WebProcessCrashed !!!");
    DALI_LOG_RELEASE_INFO("[LifecycleTest] WebProcessCrashedSignal!\n");
  }

  bool OnGeolocationPermission(WebView /*view*/, const Dali::String& origin, const Dali::String& url)
  {
    SetInfo("GeolocationPermission: origin=%s  => ALLOW", origin.CStr());
    DALI_LOG_RELEASE_INFO("[LifecycleTest] GeolocationPermission: origin=\"%s\"  url=\"%s\" -> allow\n",
                          origin.CStr(), url.CStr());
    return true; // allow
  }

  void OnPageLoadFinished(WebView /*view*/, const Dali::String& url)
  {
    DALI_LOG_RELEASE_INFO("[LifecycleTest] PageLoadFinished: %s  frames_so_far=%u\n",
                          url.CStr(), mFrameCount);
    mFrameCount = 0; // reset frame counter per page
  }

  // ---------------------------------------------------------------------------
  // Async callbacks
  // ---------------------------------------------------------------------------

  void OnVideoPlayingResult(bool isPlaying)
  {
    SetInfo("CheckVideoPlayingAsynchronously -> isPlaying=%s", isPlaying ? "true" : "false");
    DALI_LOG_RELEASE_INFO("[LifecycleTest] CheckVideoPlayingAsynchronously: isPlaying=%s\n",
                          isPlaying ? "true" : "false");
  }

  // ---------------------------------------------------------------------------
  // Key events
  // ---------------------------------------------------------------------------

  void OnKeyEvent(KeyEvent event)
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
    else if(key == "S" || key == "s")
    {
      if(!mSuspended)
      {
        mWebView.Suspend();
        mSuspended = true;
        DALI_LOG_RELEASE_INFO("[LifecycleTest] Suspend()\n");
      }
      else
      {
        mWebView.Resume();
        mSuspended = false;
        DALI_LOG_RELEASE_INFO("[LifecycleTest] Resume()\n");
      }
      UpdateStateLabel();
    }
    else if(key == "N" || key == "n")
    {
      if(!mNetSuspended)
      {
        mWebView.SuspendNetworkLoading();
        mNetSuspended = true;
        DALI_LOG_RELEASE_INFO("[LifecycleTest] SuspendNetworkLoading()\n");
      }
      else
      {
        mWebView.ResumeNetworkLoading();
        mNetSuspended = false;
        DALI_LOG_RELEASE_INFO("[LifecycleTest] ResumeNetworkLoading()\n");
      }
      UpdateStateLabel();
    }
    else if(key == "B" || key == "b")
    {
      mBgColorIdx = (mBgColorIdx + 1) % BG_COLOR_COUNT;
      const Dali::Vector4& color = BG_COLORS[mBgColorIdx];
      mWebView.SetDocumentBackgroundColor(color);
      SetInfo("SetDocumentBackgroundColor: %s  (%.1f,%.1f,%.1f,%.1f)",
              BG_COLOR_NAMES[mBgColorIdx], color.r, color.g, color.b, color.a);
      DALI_LOG_RELEASE_INFO("[LifecycleTest] SetDocumentBackgroundColor: %s  (%.2f,%.2f,%.2f,%.2f)\n",
                            BG_COLOR_NAMES[mBgColorIdx], color.r, color.g, color.b, color.a);
    }
    else if(key == "T" || key == "t")
    {
      mTilesClearedWhenHidden = !mTilesClearedWhenHidden;
      mWebView.SetTilesClearedWhenHidden(mTilesClearedWhenHidden);
      UpdateStateLabel();
      DALI_LOG_RELEASE_INFO("[LifecycleTest] SetTilesClearedWhenHidden(%s)\n",
                            mTilesClearedWhenHidden ? "true" : "false");
    }
    else if(key == "M" || key == "m")
    {
      mTileMultIdx = (mTileMultIdx + 1) % TILE_MULT_COUNT;
      float mult   = TILE_MULT_CYCLE[mTileMultIdx];
      mWebView.SetTileCoverAreaMultiplier(mult);
      UpdateStateLabel();
      DALI_LOG_RELEASE_INFO("[LifecycleTest] SetTileCoverAreaMultiplier(%.1f)\n", mult);
    }
    else if(key == "C" || key == "c")
    {
      mCursorByClient = !mCursorByClient;
      mWebView.SetCursorEnabledByClient(mCursorByClient);
      SetInfo("SetCursorEnabledByClient: %s", mCursorByClient ? "true" : "false");
      DALI_LOG_RELEASE_INFO("[LifecycleTest] SetCursorEnabledByClient(%s)\n",
                            mCursorByClient ? "true" : "false");
    }
    else if(key == "R" || key == "r")
    {
      mWebView.ClearAllTilesResources();
      SetInfo("ClearAllTilesResources() called");
      DALI_LOG_RELEASE_INFO("[LifecycleTest] ClearAllTilesResources()\n");
    }
    else if(key == "V" || key == "v")
    {
      bool current = mWebView.GetVideoHoleEnabled();
      mWebView.SetVideoHoleEnabled(!current);
      bool actual = mWebView.GetVideoHoleEnabled();
      SetInfo("SetVideoHoleEnabled(%s) -> GetVideoHoleEnabled=%s",
              !current ? "true" : "false", actual ? "true" : "false");
      DALI_LOG_RELEASE_INFO("[LifecycleTest] SetVideoHoleEnabled(%s) -> %s\n",
                            !current ? "true" : "false", actual ? "true" : "false");
      UpdateStateLabel();
    }
    else if(key == "P" || key == "p")
    {
      bool ok = mWebView.CheckVideoPlayingAsynchronously(
        WebView::VideoPlayingCallback::New(this, &LifecycleTestController::OnVideoPlayingResult));
      SetInfo("CheckVideoPlayingAsynchronously: %s — waiting...", ok ? "started" : "FAILED");
      DALI_LOG_RELEASE_INFO("[LifecycleTest] CheckVideoPlayingAsynchronously: %s\n",
                            ok ? "started" : "FAILED");
    }
    else if(key == "F" || key == "f")
    {
      if(mInFullscreen)
      {
        mWebView.ExitFullscreen();
        SetInfo("ExitFullscreen() called");
        DALI_LOG_RELEASE_INFO("[LifecycleTest] ExitFullscreen()\n");
      }
      else
      {
        SetInfo("Not in fullscreen (ExitFullscreen skipped)");
        DALI_LOG_RELEASE_INFO("[LifecycleTest] ExitFullscreen skipped — not in fullscreen\n");
      }
    }
    else if(key == "1")
    {
      mWebView.LoadHtmlString(Dali::String(VIDEO_HTML));
      SetInfo("Video test page loaded — double-click video for fullscreen");
      DALI_LOG_RELEASE_INFO("[LifecycleTest] Loaded video test page\n");
    }
    else if(key == "2")
    {
      mWebView.LoadHtmlString(Dali::String(GEO_HTML));
      SetInfo("Geolocation test page loaded — GeolocationPermission signal expected");
      DALI_LOG_RELEASE_INFO("[LifecycleTest] Loaded geolocation test page\n");
    }
    else if(key == "0")
    {
      mWebView.LoadHtmlString(Dali::String(DEFAULT_HTML));
      UpdateStateLabel();
      DALI_LOG_RELEASE_INFO("[LifecycleTest] Loaded default page\n");
    }
  }

  // ---------------------------------------------------------------------------
  // Helpers
  // ---------------------------------------------------------------------------

  void UpdateStateLabel()
  {
    float mult = TILE_MULT_CYCLE[mTileMultIdx];
    SetInfo("%s | net=%s | videoHole=%s | tilesClear=%s | mult=%.1fx",
            mSuspended ? "SUSPENDED" : "active",
            mNetSuspended ? "SUSPENDED" : "running",
            mWebView.GetVideoHoleEnabled() ? "ON" : "OFF",
            mTilesClearedWhenHidden ? "ON" : "OFF",
            mult);
  }

  template<typename... Args>
  void SetInfo(const char* fmt, Args... args)
  {
    char buf[320];
    snprintf(buf, sizeof(buf), fmt, args...);
    mInfoLabel.SetText(Dali::String(buf));
  }

private:
  Application&      mApplication;
  Label             mHintLabel;
  Label             mInfoLabel;
  Dali::Ui::WebView mWebView;

  bool     mSuspended{false};
  bool     mNetSuspended{false};
  bool     mTilesClearedWhenHidden{false};
  bool     mCursorByClient{false};
  bool     mInFullscreen{false};
  int      mBgColorIdx{0};
  int      mTileMultIdx{0};
  uint32_t mFrameCount{0};
};

int DALI_EXPORT_API main(int argc, char** argv)
{
  Application application = Application::New(&argc, &argv);
  UiConfig::New().Apply();
  LifecycleTestController test(application);
  application.MainLoop();
  return 0;
}
