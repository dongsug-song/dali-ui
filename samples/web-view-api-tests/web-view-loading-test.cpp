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
 * @file  web-view-loading-test.cpp
 * @brief Tests page loading, navigation, content info, user-agent, and custom headers.
 *
 * Key bindings
 *   1        LoadUrl  (samsung.com)
 *   2        LoadUrl  (google.com)
 *   3        LoadHtmlString  (inline HTML)
 *   4        LoadHtmlStringOverrideCurrentEntry
 *   5        LoadContents  (raw bytes, text/html)
 *   R        Reload
 *   N        ReloadWithoutCache
 *   X        StopLoading
 *   F        GoForward
 *   B        GoBack
 *   C        ClearHistory
 *   I        Log page info  (GetUrl / GetTitle / GetLoadProgressPercentage / GetFavicon)
 *   P        GetPlainTextAsynchronously
 *   U        Cycle SetUserAgent  + log GetUserAgent
 *   H        Toggle AddCustomHeader / RemoveCustomHeader  ("X-Dali-Test")
 *   ESC/BACK Quit
 */

#include <dali-ui-foundation/dali-ui-foundation.h>
#include <dali-ui-foundation/public-api/web-view.h>
#include <dali/integration-api/debug.h>
#include <cstdio>
#include <cstring>

using namespace Dali;
using namespace Dali::Ui;

namespace
{
const char* URL_1       = "https://www.samsung.com";
const char* URL_2       = "https://www.google.com";
const float INFO_HEIGHT = 32.0f;
const float HINT_HEIGHT = 28.0f;
const float FONT_HINT   = 10.0f;
const float FONT_INFO   = 11.0f;

const UiColor BG_DARK(0x12, 0x12, 0x1F, 0xFF);
const UiColor BG_INFO(0x1E, 0x1E, 0x30, 0xFF);
const UiColor FG_WHITE(0xFF, 0xFF, 0xFF, 0xFF);
const UiColor FG_GRAY(0x88, 0x88, 0xAA, 0xFF);
const UiColor FG_GREEN(0x50, 0xFA, 0x7B, 0xFF);

const char* HINT_TEXT =
  "1:samsung  2:google  3:HTML  4:Override  5:Contents  "
  "R:Reload  N:NoCache  X:Stop  F:Fwd  B:Back  C:ClearHist  "
  "I:Info  P:PlainText  U:UserAgent  H:Header";

const char* USER_AGENTS[] = {
  "DaliWebView/1.0 (Tizen)",
  "Mozilla/5.0 (Linux; Android 12) AppleWebKit/537.36",
  "Samsung SmartTV/2026 Dali Browser",
};
const int UA_COUNT = 3;

const char* CUSTOM_HEADER_NAME  = "X-Dali-Test";
const char* CUSTOM_HEADER_VALUE = "loading-test-sample";

const char* INLINE_HTML =
  "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>LoadHtmlString Test</title></head>"
  "<body style='background:#1a1a2e;color:#eee;font-family:sans-serif;padding:40px;text-align:center'>"
  "<h2>LoadHtmlString</h2>"
  "<p>This page was loaded via <b>LoadHtmlString()</b>.</p>"
  "<p id='ts'></p>"
  "<script>document.getElementById('ts').textContent='Loaded at: '+new Date().toLocaleTimeString();</script>"
  "</body></html>";

const char* OVERRIDE_HTML =
  "<!DOCTYPE html><html><head><title>Override Entry</title></head>"
  "<body style='background:#2e1a1a;color:#eee;font-family:sans-serif;padding:40px;text-align:center'>"
  "<h2>LoadHtmlStringOverrideCurrentEntry</h2>"
  "<p>base URI override demo — navigating back will skip this entry.</p>"
  "</body></html>";

const char* CONTENTS_HTML =
  "<!DOCTYPE html><html><head><title>LoadContents</title></head>"
  "<body style='background:#1a2e1a;color:#eee;font-family:sans-serif;padding:40px;text-align:center'>"
  "<h2>LoadContents Test</h2>"
  "<p>Loaded via <b>LoadContents()</b> with MIME type <code>text/html</code>.</p>"
  "</body></html>";
} // namespace

class LoadingTestController : public ConnectionTracker
{
public:
  explicit LoadingTestController(Application& application)
  : mApplication(application)
  {
    mApplication.InitSignal().Connect(this, &LoadingTestController::OnInit);
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

    mInfoLabel = Label::New("Status: ready")
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

    mWebView.PageLoadStartedSignal().Connect(this, &LoadingTestController::OnPageLoadStarted);
    mWebView.PageLoadInProgressSignal().Connect(this, &LoadingTestController::OnPageLoadInProgress);
    mWebView.PageLoadFinishedSignal().Connect(this, &LoadingTestController::OnPageLoadFinished);
    mWebView.UrlChangedSignal().Connect(this, &LoadingTestController::OnUrlChanged);

    window.KeyEventSignal().Connect(this, &LoadingTestController::OnKeyEvent);

    mWebView.LoadUrl(Dali::String(URL_1));
    SetInfo("Loading: %s", URL_1);
    DALI_LOG_RELEASE_INFO("[LoadingTest] Init complete. Loading %s\n", URL_1);
  }

  // ---------------------------------------------------------------------------
  // Signal handlers
  // ---------------------------------------------------------------------------

  void OnPageLoadStarted(WebView /*view*/, const Dali::String& url)
  {
    SetInfo("Loading started: %s", url.CStr());
    DALI_LOG_RELEASE_INFO("[LoadingTest] PageLoadStarted: %s\n", url.CStr());
  }

  void OnPageLoadInProgress(WebView /*view*/, const Dali::String& url)
  {
    float pct = mWebView.GetLoadProgressPercentage();
    SetInfo("In progress %.0f%%: %s", pct, url.CStr());
    DALI_LOG_RELEASE_INFO("[LoadingTest] PageLoadInProgress %.0f%%: %s\n", pct, url.CStr());
  }

  void OnPageLoadFinished(WebView /*view*/, const Dali::String& url)
  {
    Dali::String title = mWebView.GetTitle();
    SetInfo("Loaded | title=%s", title.CStr());
    DALI_LOG_RELEASE_INFO("[LoadingTest] PageLoadFinished: %s  title=\"%s\"\n", url.CStr(), title.CStr());
  }

  void OnUrlChanged(WebView /*view*/, const Dali::String& url)
  {
    DALI_LOG_RELEASE_INFO("[LoadingTest] UrlChanged: %s\n", url.CStr());
  }

  // ---------------------------------------------------------------------------
  // Async callbacks
  // ---------------------------------------------------------------------------

  void OnPlainText(const Dali::String& text)
  {
    DALI_LOG_RELEASE_INFO("[LoadingTest] PlainText (%u chars): %.120s...\n",
                          static_cast<unsigned>(text.Length()), text.CStr());
    SetInfo("PlainText: %u chars", static_cast<unsigned>(text.Length()));
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
    else if(key == "1")
    {
      mWebView.LoadUrl(Dali::String(URL_1));
      SetInfo("LoadUrl: %s", URL_1);
      DALI_LOG_RELEASE_INFO("[LoadingTest] LoadUrl: %s\n", URL_1);
    }
    else if(key == "2")
    {
      mWebView.LoadUrl(Dali::String(URL_2));
      SetInfo("LoadUrl: %s", URL_2);
      DALI_LOG_RELEASE_INFO("[LoadingTest] LoadUrl: %s\n", URL_2);
    }
    else if(key == "3")
    {
      mWebView.LoadHtmlString(Dali::String(INLINE_HTML));
      SetInfo("LoadHtmlString: inline HTML");
      DALI_LOG_RELEASE_INFO("[LoadingTest] LoadHtmlString\n");
    }
    else if(key == "4")
    {
      bool ok = mWebView.LoadHtmlStringOverrideCurrentEntry(
        Dali::String(OVERRIDE_HTML),
        Dali::String("https://override.test/"),
        Dali::String("https://unreachable.test/"));
      SetInfo("LoadHtmlStringOverrideCurrentEntry: %s", ok ? "OK" : "FAIL");
      DALI_LOG_RELEASE_INFO("[LoadingTest] LoadHtmlStringOverrideCurrentEntry: %s\n", ok ? "OK" : "FAIL");
    }
    else if(key == "5")
    {
      const int8_t* data = reinterpret_cast<const int8_t*>(CONTENTS_HTML);
      uint32_t      size = static_cast<uint32_t>(strlen(CONTENTS_HTML));
      bool          ok   = mWebView.LoadContents(data, size,
                                        Dali::String("text/html"),
                                        Dali::String("UTF-8"),
                                        Dali::String("https://contents.test/"));
      SetInfo("LoadContents: %s (%u bytes)", ok ? "OK" : "FAIL", size);
      DALI_LOG_RELEASE_INFO("[LoadingTest] LoadContents: %s  size=%u\n", ok ? "OK" : "FAIL", size);
    }
    else if(key == "R" || key == "r")
    {
      mWebView.Reload();
      SetInfo("Reload");
      DALI_LOG_RELEASE_INFO("[LoadingTest] Reload\n");
    }
    else if(key == "N" || key == "n")
    {
      bool ok = mWebView.ReloadWithoutCache();
      SetInfo("ReloadWithoutCache: %s", ok ? "OK" : "FAIL");
      DALI_LOG_RELEASE_INFO("[LoadingTest] ReloadWithoutCache: %s\n", ok ? "OK" : "FAIL");
    }
    else if(key == "X" || key == "x")
    {
      mWebView.StopLoading();
      SetInfo("StopLoading");
      DALI_LOG_RELEASE_INFO("[LoadingTest] StopLoading\n");
    }
    else if(key == "F" || key == "f")
    {
      if(mWebView.CanGoForward())
      {
        mWebView.GoForward();
        SetInfo("GoForward");
        DALI_LOG_RELEASE_INFO("[LoadingTest] GoForward\n");
      }
      else
      {
        SetInfo("CanGoForward: false");
        DALI_LOG_RELEASE_INFO("[LoadingTest] CanGoForward: false\n");
      }
    }
    else if(key == "B" || key == "b")
    {
      if(mWebView.CanGoBack())
      {
        mWebView.GoBack();
        SetInfo("GoBack");
        DALI_LOG_RELEASE_INFO("[LoadingTest] GoBack\n");
      }
      else
      {
        SetInfo("CanGoBack: false");
        DALI_LOG_RELEASE_INFO("[LoadingTest] CanGoBack: false\n");
      }
    }
    else if(key == "C" || key == "c")
    {
      mWebView.ClearHistory();
      SetInfo("ClearHistory — CanGoBack=%s CanGoFwd=%s",
              mWebView.CanGoBack() ? "T" : "F",
              mWebView.CanGoForward() ? "T" : "F");
      DALI_LOG_RELEASE_INFO("[LoadingTest] ClearHistory: CanGoBack=%s CanGoFwd=%s\n",
                            mWebView.CanGoBack() ? "true" : "false",
                            mWebView.CanGoForward() ? "true" : "false");
    }
    else if(key == "I" || key == "i")
    {
      Dali::String url    = mWebView.GetUrl();
      Dali::String title  = mWebView.GetTitle();
      float        pct    = mWebView.GetLoadProgressPercentage();
      auto         fav    = mWebView.GetFavicon();
      bool         hasFav = static_cast<bool>(fav);
      SetInfo("URL=%s | title=%s | prog=%.0f%% | favicon=%s",
              url.CStr(), title.CStr(), pct, hasFav ? "yes" : "no");
      DALI_LOG_RELEASE_INFO("[LoadingTest] GetUrl=\"%s\"  GetTitle=\"%s\"  progress=%.0f%%  favicon=%s\n",
                            url.CStr(), title.CStr(), pct, hasFav ? "yes" : "no");
    }
    else if(key == "P" || key == "p")
    {
      mWebView.GetPlainTextAsynchronously(
        WebView::PlainTextCallback::New(this, &LoadingTestController::OnPlainText));
      SetInfo("GetPlainTextAsynchronously — waiting...");
      DALI_LOG_RELEASE_INFO("[LoadingTest] GetPlainTextAsynchronously called\n");
    }
    else if(key == "U" || key == "u")
    {
      const char* ua = USER_AGENTS[mUaIndex % UA_COUNT];
      ++mUaIndex;
      mWebView.SetUserAgent(Dali::String(ua));
      Dali::String current = mWebView.GetUserAgent();
      SetInfo("UserAgent: %s", current.CStr());
      DALI_LOG_RELEASE_INFO("[LoadingTest] SetUserAgent -> GetUserAgent=\"%s\"\n", current.CStr());
    }
    else if(key == "H" || key == "h")
    {
      if(!mHeaderAdded)
      {
        bool ok = mWebView.AddCustomHeader(Dali::String(CUSTOM_HEADER_NAME),
                                           Dali::String(CUSTOM_HEADER_VALUE));
        mHeaderAdded = ok;
        SetInfo("AddCustomHeader %s: %s=%s", ok ? "OK" : "FAIL",
                CUSTOM_HEADER_NAME, CUSTOM_HEADER_VALUE);
        DALI_LOG_RELEASE_INFO("[LoadingTest] AddCustomHeader(%s: %s): %s\n",
                              CUSTOM_HEADER_NAME, CUSTOM_HEADER_VALUE, ok ? "OK" : "FAIL");
      }
      else
      {
        bool ok = mWebView.RemoveCustomHeader(Dali::String(CUSTOM_HEADER_NAME));
        mHeaderAdded = !ok;
        SetInfo("RemoveCustomHeader %s: %s", CUSTOM_HEADER_NAME, ok ? "OK" : "FAIL");
        DALI_LOG_RELEASE_INFO("[LoadingTest] RemoveCustomHeader(%s): %s\n",
                              CUSTOM_HEADER_NAME, ok ? "OK" : "FAIL");
      }
    }
  }

  // ---------------------------------------------------------------------------
  // Helpers
  // ---------------------------------------------------------------------------

  template<typename... Args>
  void SetInfo(const char* fmt, Args... args)
  {
    char buf[256];
    snprintf(buf, sizeof(buf), fmt, args...);
    mInfoLabel.SetText(Dali::String(buf));
  }

private:
  Application&   mApplication;
  Label          mHintLabel;
  Label          mInfoLabel;
  Dali::Ui::WebView mWebView;

  int  mUaIndex{0};
  bool mHeaderAdded{false};
};

int DALI_EXPORT_API main(int argc, char** argv)
{
  Application application = Application::New(&argc, &argv);
  UiConfig::New().Apply();
  LoadingTestController test(application);
  application.MainLoop();
  return 0;
}
