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
 * @file  web-browser-example.cpp
 * @brief Simple web browser built with dali-ui WebView.
 *
 * Layout
 *   +-------------------------------------------------+
 *   | [Back] [Forward] [Reload] [URL bar ......] [Go] |  toolbar
 *   +-------------------------------------------------+
 *   |  title / status                                |  status bar
 *   +-------------------------------------------------+
 *   |                                                |
 *   |                  WebView                       |
 *   |                                                |
 *   +-------------------------------------------------+
 *
 * Touch the URL bar to edit, then press [Go] to navigate.
 * ESC / BACK key quits.
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
const char* HOME_URL       = "https://www.samsung.com";
const float TOOLBAR_HEIGHT = 52.0f;
const float STATUS_HEIGHT  = 28.0f;
const float NAV_BTN_WIDTH  = 78.0f;
const float GO_BTN_WIDTH   = 48.0f;
const float FONT_SIZE_BTN  = 14.0f;
const float FONT_SIZE_URL  = 13.0f;
const float FONT_SIZE_STS  = 11.0f;

// Dark-themed palette
const UiColor COLOR_BG_TOOLBAR(0x1E, 0x1E, 0x2E, 0xFF);
const UiColor COLOR_BG_STATUS (0x13, 0x13, 0x20, 0xFF);
const UiColor COLOR_BG_URL    (0x2A, 0x2A, 0x3E, 0xFF);
const UiColor COLOR_BTN_NORMAL(0x31, 0x31, 0x4A, 0xFF);
const UiColor COLOR_BTN_PRESS (0x56, 0x56, 0x80, 0xFF);
const UiColor COLOR_FG_WHITE  (0xFF, 0xFF, 0xFF, 0xFF);
const UiColor COLOR_FG_GRAY   (0x99, 0x99, 0xBB, 0xFF);
const UiColor COLOR_FG_ACCENT (0x89, 0xB4, 0xFA, 0xFF);
const UiColor COLOR_PROGRESS  (0x89, 0xB4, 0xFA, 0x60);

Label MakeNavButton(const char* text)
{
  return Label::New(text)
    .SetFontSize(FONT_SIZE_BTN)
    .SetHorizontalTextAlignment(Text::Alignment::CENTER)
    .SetVerticalTextAlignment(Text::Alignment::CENTER)
    .SetTextColor(COLOR_FG_WHITE)
    .SetBackgroundColor(COLOR_BTN_NORMAL)
    .SetRequestedWidth(NAV_BTN_WIDTH)
    .SetRequestedHeight(TOOLBAR_HEIGHT)
    .SetFocusable(true);
}

} // namespace

class BrowserController : public ConnectionTracker
{
public:
  explicit BrowserController(Application& application)
  : mApplication(application)
  {
    mApplication.InitSignal().Connect(this, [this](Application application) {
      OnInit(application);
    });
  }

private:
  void OnInit(Application& application)
  {
    Window window = application.GetWindow();
    window.SetBackgroundColor(Color::BLACK);

    BuildUI(window);
    ConnectSignals(window);

    Navigate(Dali::String(HOME_URL));
  }

  // ---------------------------------------------------------------------------
  // UI construction
  // ---------------------------------------------------------------------------

  void BuildUI(Window& window)
  {
    // --- Nav buttons ---------------------------------------------------------
    mBtnBack    = MakeNavButton("Back");
    mBtnForward = MakeNavButton("Forward");
    mBtnReload  = MakeNavButton("Reload");

    // --- URL bar -------------------------------------------------------------
    mUrlBar = InputField::New()
      .SetPlaceholder("Enter URL")
      .SetPlaceholderColor(COLOR_FG_GRAY)
      .SetText(Dali::String(HOME_URL))
      .SetFontSize(FONT_SIZE_URL)
      .SetTextColor(COLOR_FG_WHITE)
      .SetBackgroundColor(COLOR_BG_URL)
      .SetCursorWidth(2)
      .SetCursorColor(COLOR_FG_ACCENT)
      .SetSelectionColor(COLOR_PROGRESS)
      .SetRequestedWidth(0.0f)
      .SetRequestedHeight(TOOLBAR_HEIGHT)
      .SetPadding(Extents(10, 10, 0, 0))
      .SetVerticalTextAlignment(Text::Alignment::CENTER)
      .SetFocusable(true)
      .SetLayoutParams(StackLayoutParams::New()
                         .SetWeight(1.0f)
                         .SetAlignment(LayoutAlignment::FILL));

    // --- Go button -----------------------------------------------------------
    mBtnGo = Label::New("Go")
      .SetFontSize(FONT_SIZE_BTN)
      .SetHorizontalTextAlignment(Text::Alignment::CENTER)
      .SetVerticalTextAlignment(Text::Alignment::CENTER)
      .SetTextColor(COLOR_FG_ACCENT)
      .SetBackgroundColor(COLOR_BTN_NORMAL)
      .SetRequestedWidth(GO_BTN_WIDTH)
      .SetRequestedHeight(TOOLBAR_HEIGHT)
      .SetFocusable(true);

    // --- Toolbar -------------------------------------------------------------
    StackLayout toolbar = StackLayout::New(StackOrientation::HORIZONTAL)
      .SetRequestedWidth(MATCH_PARENT)
      .SetRequestedHeight(TOOLBAR_HEIGHT)
      .SetSpacing(2.0f)
      .SetBackgroundColor(COLOR_BG_TOOLBAR)
      .Children({mBtnBack, mBtnForward, mBtnReload, mUrlBar, mBtnGo});

    // --- Status bar ----------------------------------------------------------
    mStatusLabel = Label::New("Ready")
      .SetFontSize(FONT_SIZE_STS)
      .SetHorizontalTextAlignment(Text::Alignment::START)
      .SetVerticalTextAlignment(Text::Alignment::CENTER)
      .SetTextColor(COLOR_FG_GRAY)
      .SetBackgroundColor(COLOR_BG_STATUS)
      .SetRequestedWidth(MATCH_PARENT)
      .SetRequestedHeight(STATUS_HEIGHT)
      .SetPadding(Extents(8, 8, 0, 0));

    // --- WebView -------------------------------------------------------------
    mWebView = WebView::New()
      .SetRequestedWidth(MATCH_PARENT)
      .SetRequestedHeight(0.0f)
      .SetLayoutParams(StackLayoutParams::New()
                         .SetWeight(1.0f)
                         .SetAlignment(LayoutAlignment::FILL));

    // --- Root layout ---------------------------------------------------------
    StackLayout root = StackLayout::New(StackOrientation::VERTICAL)
      .SetRequestedWidth(MATCH_PARENT)
      .SetRequestedHeight(MATCH_PARENT)
      .Children({toolbar, mStatusLabel, mWebView});

    window.Add(root);
  }

  // ---------------------------------------------------------------------------
  // Signal connections
  // ---------------------------------------------------------------------------

  void ConnectSignals(Window& window)
  {
    mBtnBack.TouchedSignal().Connect(this, [this](Actor actor, const TouchEvent& touch) {
      return OnBackTouched(actor, touch);
    });
    mBtnForward.TouchedSignal().Connect(this, [this](Actor actor, const TouchEvent& touch) {
      return OnForwardTouched(actor, touch);
    });
    mBtnReload.TouchedSignal().Connect(this, [this](Actor actor, const TouchEvent& touch) {
      return OnReloadTouched(actor, touch);
    });
    mBtnGo.TouchedSignal().Connect(this, [this](Actor actor, const TouchEvent& touch) {
      return OnGoTouched(actor, touch);
    });

    mWebView.PageLoadStartedSignal().Connect(this, &BrowserController::OnPageLoadStarted);
    mWebView.PageLoadInProgressSignal().Connect(this, &BrowserController::OnPageLoadInProgress);
    mWebView.PageLoadFinishedSignal().Connect(this, &BrowserController::OnPageLoadFinished);
    mWebView.UrlChangedSignal().Connect(this, &BrowserController::OnUrlChanged);

    window.KeyEventSignal().Connect(this, [this](const KeyEvent& event) {
      OnKeyEvent(event);
    });
  }

  // ---------------------------------------------------------------------------
  // Navigation
  // ---------------------------------------------------------------------------

  void Navigate(Dali::String url)
  {
    if(url.Empty())
    {
      return;
    }
    // Prepend https:// if no scheme is present
    if(!strstr(url.CStr(), "://"))
    {
      url = Dali::String("https://") + url;
    }
    mWebView.LoadUrl(url);
    mUrlBar.SetText(url);
    UpdateNavButtons();
  }

  void UpdateNavButtons()
  {
    mBtnBack.SetTextColor(mWebView.CanGoBack() ? COLOR_FG_WHITE : COLOR_FG_GRAY);
    mBtnForward.SetTextColor(mWebView.CanGoForward() ? COLOR_FG_WHITE : COLOR_FG_GRAY);
    mBtnReload.SetText(mIsLoading ? Dali::String("Stop") : Dali::String("Reload"));
  }

  void SetStatus(const Dali::String& text)
  {
    mStatusLabel.SetText(text);
  }

  // ---------------------------------------------------------------------------
  // Button touch handlers
  // ---------------------------------------------------------------------------

  bool OnBackTouched(Actor /*actor*/, const TouchEvent& touch)
  {
    if(touch.GetState(0) == PointState::UP)
    {
      if(mWebView.CanGoBack())
      {
        mWebView.GoBack();
        UpdateNavButtons();
      }
    }
    return true;
  }

  bool OnForwardTouched(Actor /*actor*/, const TouchEvent& touch)
  {
    if(touch.GetState(0) == PointState::UP)
    {
      if(mWebView.CanGoForward())
      {
        mWebView.GoForward();
        UpdateNavButtons();
      }
    }
    return true;
  }

  bool OnReloadTouched(Actor /*actor*/, const TouchEvent& touch)
  {
    if(touch.GetState(0) == PointState::UP)
    {
      if(mIsLoading)
      {
        mWebView.StopLoading();
        mIsLoading = false;
        SetStatus(Dali::String("Stopped"));
      }
      else
      {
        mWebView.Reload();
      }
      UpdateNavButtons();
    }
    return true;
  }

  bool OnGoTouched(Actor /*actor*/, const TouchEvent& touch)
  {
    if(touch.GetState(0) == PointState::UP)
    {
      Navigate(mUrlBar.GetText());
    }
    return true;
  }

  // ---------------------------------------------------------------------------
  // WebView signal handlers
  // ---------------------------------------------------------------------------

  void OnPageLoadStarted(WebView /*view*/, const Dali::String& url)
  {
    mIsLoading = true;
    SetStatus(Dali::String("Loading..."));
    UpdateNavButtons();
    DALI_LOG_RELEASE_INFO("[Browser] Loading: %s\n", url.CStr());
  }

  void OnPageLoadInProgress(WebView /*view*/, const Dali::String& url)
  {
    float pct = mWebView.GetLoadProgressPercentage();
    char  buf[64];
    snprintf(buf, sizeof(buf), "Loading  %.0f%%", pct);
    SetStatus(Dali::String(buf));
  }

  void OnPageLoadFinished(WebView /*view*/, const Dali::String& url)
  {
    mIsLoading = false;
    Dali::String title = mWebView.GetTitle();
    SetStatus(title.Empty() ? url : title);
    UpdateNavButtons();
    DALI_LOG_RELEASE_INFO("[Browser] Loaded: %s  title=\"%s\"\n", url.CStr(), title.CStr());
  }

  void OnUrlChanged(WebView /*view*/, const Dali::String& url)
  {
    mUrlBar.SetText(url);
  }

  // ---------------------------------------------------------------------------
  // Key events (window-level)
  // ---------------------------------------------------------------------------

  void OnKeyEvent(const KeyEvent& event)
  {
    if(event.GetState() != KeyEvent::DOWN)
    {
      return;
    }

    if(IsKey(event, Dali::DALI_KEY_ESCAPE) || IsKey(event, Dali::DALI_KEY_BACK))
    {
      mApplication.Quit();
    }
    else if(event.GetKeyName() == "Return")
    {
      Navigate(mUrlBar.GetText());
    }
  }

private:
  Application& mApplication;

  // Toolbar widgets
  Label      mBtnBack;
  Label      mBtnForward;
  Label      mBtnReload;
  InputField mUrlBar;
  Label      mBtnGo;

  // Content area
  Label   mStatusLabel;
  WebView mWebView;

  bool mIsLoading{false};
};

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

int DALI_EXPORT_API main(int argc, char** argv)
{
  Application    application = Application::New(&argc, &argv);
  UiConfig::New().Apply();
  BrowserController browser(application);
  application.MainLoop();
  return 0;
}
