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
 * @file  web-view-scroll-zoom-test.cpp
 * @brief Tests scroll (ScrollBy, ScrollEdgeBy, GetScroll*) and
 *        zoom/scale (SetPageZoomFactor, SetTextZoomFactor, SetScaleFactor) APIs.
 *
 * Key bindings
 *   Arrow Up/Down/Left/Right    ScrollBy ±100 px
 *   Page Up / Page Down         ScrollBy 0, ±300 px
 *   Home                        ScrollBy to origin (0,0 via JS)
 *   E                           ScrollEdgeBy (push to bottom edge)
 *   +  / =                      SetPageZoomFactor +0.25
 *   -                           SetPageZoomFactor -0.25
 *   T                           Cycle SetTextZoomFactor (0.5→1.0→1.5→2.0)
 *   Z                           Log all zoom/scale values
 *   I                           Log scroll info (position, scrollSize, contentSize)
 *   S                           SetScaleFactor cycle (0.5→1.0→1.5→2.0) at center
 *   R                           Reload test page
 *   ESC/BACK                    Quit
 *
 * Signals verified: ScrollEdgeReachedSignal, OverScrolledSignal
 */

#include <dali-ui-foundation/dali-ui-foundation.h>
#include <dali-ui-foundation/public-api/web-view.h>
#include <dali/integration-api/debug.h>
#include <cstdio>

using namespace Dali;
using namespace Dali::Ui;

namespace
{
const float INFO_HEIGHT = 48.0f; // two-line info area
const float HINT_HEIGHT = 28.0f;
const float FONT_HINT   = 10.0f;
const float FONT_INFO   = 11.0f;

const UiColor BG_DARK(0x12, 0x12, 0x1F, 0xFF);
const UiColor BG_INFO(0x1E, 0x1E, 0x30, 0xFF);
const UiColor FG_WHITE(0xFF, 0xFF, 0xFF, 0xFF);
const UiColor FG_GRAY(0x88, 0x88, 0xAA, 0xFF);

const char* HINT_TEXT =
  "Arrows:scroll±100  PgUp/Dn:±300  E:ScrollEdgeBy  +/-:PageZoom  T:TextZoom  S:Scale  Z:LogZoom  I:LogScroll";

const float ZOOM_STEPS[] = {0.5f, 0.75f, 1.0f, 1.25f, 1.5f, 1.75f, 2.0f};
const int   ZOOM_STEP_COUNT = 7;
const int   ZOOM_DEFAULT_IDX = 2; // 1.0

const float TEXT_ZOOM_CYCLE[] = {0.5f, 1.0f, 1.5f, 2.0f};
const int   TEXT_ZOOM_COUNT = 4;

const float SCALE_CYCLE[] = {0.5f, 1.0f, 1.5f, 2.0f};
const int   SCALE_COUNT = 4;

const char* SCROLL_TEST_HTML =
  "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Scroll/Zoom Test</title>"
  "<style>"
  "body{margin:0;padding:0;background:#1a1a2e;overflow:scroll}"
  ".grid{display:grid;grid-template-columns:repeat(8,200px);gap:2px;padding:10px}"
  ".cell{width:200px;height:200px;display:flex;align-items:center;justify-content:center;"
  "font-family:monospace;font-size:14px;border:1px solid #333}"
  "</style></head>"
  "<body>"
  "<div class='grid' id='g'></div>"
  "<script>"
  "var g=document.getElementById('g');"
  "var colors=['#16213e','#1a1a2e','#0f3460','#533483','#1b262c'];"
  "for(var r=0;r<12;r++){"
  "  for(var c=0;c<8;c++){"
  "    var d=document.createElement('div');"
  "    d.className='cell';"
  "    d.style.background=colors[(r+c)%colors.length];"
  "    d.style.color='#ccc';"
  "    d.textContent='['+r+','+c+']';"
  "    g.appendChild(d);"
  "  }"
  "}"
  "</script>"
  "</body></html>";
} // namespace

class ScrollZoomTestController : public ConnectionTracker
{
public:
  explicit ScrollZoomTestController(Application& application)
  : mApplication(application)
  {
    mApplication.InitSignal().Connect(this, &ScrollZoomTestController::OnInit);
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

    mInfoLabel = Label::New("scroll=(0,0)  zoom=1.0  textZoom=1.0  scale=1.0")
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

    mWebView.ScrollEdgeReachedSignal().Connect(this, &ScrollZoomTestController::OnScrollEdgeReached);
    mWebView.OverScrolledSignal().Connect(this, &ScrollZoomTestController::OnOverScrolled);
    mWebView.PageLoadFinishedSignal().Connect(this, &ScrollZoomTestController::OnPageLoadFinished);

    window.KeyEventSignal().Connect(this, &ScrollZoomTestController::OnKeyEvent);

    mWebView.LoadHtmlString(Dali::String(SCROLL_TEST_HTML));
    DALI_LOG_RELEASE_INFO("[ScrollZoomTest] Init complete\n");
  }

  // ---------------------------------------------------------------------------
  // Signal handlers
  // ---------------------------------------------------------------------------

  void OnPageLoadFinished(WebView /*view*/, const Dali::String& /*url*/)
  {
    LogScrollInfo();
    LogZoomInfo();
  }

  void OnScrollEdgeReached(WebView /*view*/, WebViewScrollEdge edge)
  {
    const char* edgeName = "UNKNOWN";
    switch(edge)
    {
      case WebViewScrollEdge::LEFT:   edgeName = "LEFT";   break;
      case WebViewScrollEdge::RIGHT:  edgeName = "RIGHT";  break;
      case WebViewScrollEdge::TOP:    edgeName = "TOP";    break;
      case WebViewScrollEdge::BOTTOM: edgeName = "BOTTOM"; break;
    }
    SetInfo("ScrollEdgeReached: %s", edgeName);
    DALI_LOG_RELEASE_INFO("[ScrollZoomTest] ScrollEdgeReached: %s\n", edgeName);
  }

  void OnOverScrolled(WebView /*view*/, WebViewOverScrolled direction)
  {
    const char* dirName = "UNKNOWN";
    switch(direction)
    {
      case WebViewOverScrolled::LEFT:   dirName = "LEFT";   break;
      case WebViewOverScrolled::RIGHT:  dirName = "RIGHT";  break;
      case WebViewOverScrolled::TOP:    dirName = "TOP";    break;
      case WebViewOverScrolled::BOTTOM: dirName = "BOTTOM"; break;
    }
    SetInfo("OverScrolled: %s", dirName);
    DALI_LOG_RELEASE_INFO("[ScrollZoomTest] OverScrolled: %s\n", dirName);
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
    else if(key == "Up")
    {
      mWebView.ScrollBy(0, -100);
      LogScrollInfo();
      DALI_LOG_RELEASE_INFO("[ScrollZoomTest] ScrollBy(0, -100)\n");
    }
    else if(key == "Down")
    {
      mWebView.ScrollBy(0, 100);
      LogScrollInfo();
      DALI_LOG_RELEASE_INFO("[ScrollZoomTest] ScrollBy(0, 100)\n");
    }
    else if(key == "Left")
    {
      mWebView.ScrollBy(-100, 0);
      LogScrollInfo();
      DALI_LOG_RELEASE_INFO("[ScrollZoomTest] ScrollBy(-100, 0)\n");
    }
    else if(key == "Right")
    {
      mWebView.ScrollBy(100, 0);
      LogScrollInfo();
      DALI_LOG_RELEASE_INFO("[ScrollZoomTest] ScrollBy(100, 0)\n");
    }
    else if(key == "Prior") // Page Up
    {
      mWebView.ScrollBy(0, -300);
      LogScrollInfo();
      DALI_LOG_RELEASE_INFO("[ScrollZoomTest] ScrollBy(0, -300)\n");
    }
    else if(key == "Next") // Page Down
    {
      mWebView.ScrollBy(0, 300);
      LogScrollInfo();
      DALI_LOG_RELEASE_INFO("[ScrollZoomTest] ScrollBy(0, 300)\n");
    }
    else if(key == "Home")
    {
      // Scroll back to origin via large negative delta
      mWebView.EvaluateJavaScript(Dali::String("window.scrollTo(0,0);"));
      SetInfo("EvaluateJavaScript: window.scrollTo(0,0)");
      DALI_LOG_RELEASE_INFO("[ScrollZoomTest] ScrollTo origin via JS\n");
    }
    else if(key == "E" || key == "e")
    {
      // Push toward bottom-right edge
      bool ok = mWebView.ScrollEdgeBy(9999, 9999);
      Dali::Vector2 pos = mWebView.GetScrollPosition();
      SetInfo("ScrollEdgeBy(9999,9999): %s  pos=(%.0f,%.0f)", ok ? "OK" : "FAIL", pos.x, pos.y);
      DALI_LOG_RELEASE_INFO("[ScrollZoomTest] ScrollEdgeBy(9999,9999): %s  pos=(%.0f,%.0f)\n",
                            ok ? "OK" : "FAIL", pos.x, pos.y);
    }
    else if(key == "plus" || key == "equal")
    {
      mPageZoomIdx = (mPageZoomIdx + 1) % ZOOM_STEP_COUNT;
      float z = ZOOM_STEPS[mPageZoomIdx];
      mWebView.SetPageZoomFactor(z);
      float actual = mWebView.GetPageZoomFactor();
      SetInfo("SetPageZoomFactor(%.2f) -> GetPageZoomFactor=%.2f", z, actual);
      DALI_LOG_RELEASE_INFO("[ScrollZoomTest] SetPageZoomFactor(%.2f) -> %.2f\n", z, actual);
    }
    else if(key == "minus")
    {
      mPageZoomIdx = (mPageZoomIdx + ZOOM_STEP_COUNT - 1) % ZOOM_STEP_COUNT;
      float z = ZOOM_STEPS[mPageZoomIdx];
      mWebView.SetPageZoomFactor(z);
      float actual = mWebView.GetPageZoomFactor();
      SetInfo("SetPageZoomFactor(%.2f) -> GetPageZoomFactor=%.2f", z, actual);
      DALI_LOG_RELEASE_INFO("[ScrollZoomTest] SetPageZoomFactor(%.2f) -> %.2f\n", z, actual);
    }
    else if(key == "T" || key == "t")
    {
      mTextZoomIdx = (mTextZoomIdx + 1) % TEXT_ZOOM_COUNT;
      float z = TEXT_ZOOM_CYCLE[mTextZoomIdx];
      mWebView.SetTextZoomFactor(z);
      float actual = mWebView.GetTextZoomFactor();
      SetInfo("SetTextZoomFactor(%.2f) -> GetTextZoomFactor=%.2f", z, actual);
      DALI_LOG_RELEASE_INFO("[ScrollZoomTest] SetTextZoomFactor(%.2f) -> %.2f\n", z, actual);
    }
    else if(key == "S" || key == "s")
    {
      mScaleIdx = (mScaleIdx + 1) % SCALE_COUNT;
      float sc = SCALE_CYCLE[mScaleIdx];
      Dali::Vector2 center(
        static_cast<float>(mWindowSize.GetWidth()) * 0.5f,
        static_cast<float>(mWindowSize.GetHeight()) * 0.5f);
      mWebView.SetScaleFactor(sc, center);
      float actual = mWebView.GetScaleFactor();
      SetInfo("SetScaleFactor(%.2f, center) -> GetScaleFactor=%.2f", sc, actual);
      DALI_LOG_RELEASE_INFO("[ScrollZoomTest] SetScaleFactor(%.2f) -> %.2f\n", sc, actual);
    }
    else if(key == "Z" || key == "z")
    {
      LogZoomInfo();
    }
    else if(key == "I" || key == "i")
    {
      LogScrollInfo();
    }
    else if(key == "R" || key == "r")
    {
      mWebView.LoadHtmlString(Dali::String(SCROLL_TEST_HTML));
      DALI_LOG_RELEASE_INFO("[ScrollZoomTest] Test page reloaded\n");
    }
  }

  // ---------------------------------------------------------------------------
  // Logging helpers
  // ---------------------------------------------------------------------------

  void LogScrollInfo()
  {
    Dali::Vector2 pos     = mWebView.GetScrollPosition();
    Dali::Vector2 scrSize = mWebView.GetScrollSize();
    Dali::Vector2 conSize = mWebView.GetContentSize();
    SetInfo("pos=(%.0f,%.0f)  scrollSz=(%.0f,%.0f)  contentSz=(%.0f,%.0f)",
            pos.x, pos.y, scrSize.x, scrSize.y, conSize.x, conSize.y);
    DALI_LOG_RELEASE_INFO("[ScrollZoomTest] pos=(%.0f,%.0f)  scrollSize=(%.0f,%.0f)  contentSize=(%.0f,%.0f)\n",
                          pos.x, pos.y, scrSize.x, scrSize.y, conSize.x, conSize.y);
  }

  void LogZoomInfo()
  {
    float pageZoom = mWebView.GetPageZoomFactor();
    float textZoom = mWebView.GetTextZoomFactor();
    float scale    = mWebView.GetScaleFactor();
    SetInfo("pageZoom=%.2f  textZoom=%.2f  scale=%.2f", pageZoom, textZoom, scale);
    DALI_LOG_RELEASE_INFO("[ScrollZoomTest] GetPageZoomFactor=%.2f  GetTextZoomFactor=%.2f  GetScaleFactor=%.2f\n",
                          pageZoom, textZoom, scale);
  }

  template<typename... Args>
  void SetInfo(const char* fmt, Args... args)
  {
    char buf[256];
    snprintf(buf, sizeof(buf), fmt, args...);
    mInfoLabel.SetText(Dali::String(buf));
  }

private:
  Application&      mApplication;
  Label             mHintLabel;
  Label             mInfoLabel;
  Dali::Ui::WebView mWebView;
  Window::WindowSize mWindowSize{1920, 1080};

  int mPageZoomIdx{ZOOM_DEFAULT_IDX};
  int mTextZoomIdx{1}; // 1.0
  int mScaleIdx{1};    // 1.0
};

int DALI_EXPORT_API main(int argc, char** argv)
{
  Application application = Application::New(&argc, &argv);
  UiConfig::New().Apply();
  ScrollZoomTestController test(application);
  application.MainLoop();
  return 0;
}
