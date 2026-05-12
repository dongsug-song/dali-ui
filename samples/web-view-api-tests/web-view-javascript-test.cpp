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
 * @file  web-view-javascript-test.cpp
 * @brief Tests all JavaScript integration APIs and dialog callbacks.
 *
 * Key bindings
 *   1        EvaluateJavaScript  (no callback) — change background color
 *   2        EvaluateJavaScript  (with callback) — get document.title
 *   3        EvaluateJavaScript  (with callback) — get navigator.userAgent
 *   4        EvaluateJavaScript  — inject message-send button (AddJavaScriptMessageHandler)
 *   5        EvaluateJavaScript  — trigger JS alert()
 *   6        EvaluateJavaScript  — trigger JS confirm()
 *   7        EvaluateJavaScript  — trigger JS prompt()
 *   S        GetSelectedText
 *   R        Reload test page
 *   ESC/BACK Quit
 */

#include <dali-ui-foundation/dali-ui-foundation.h>
#include <dali-ui-foundation/public-api/web-view.h>
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
  "1:EvalJS(no-cb)  2:EvalJS(title)  3:EvalJS(ua)  4:MsgHandler  "
  "5:Alert  6:Confirm  7:Prompt  S:SelText  R:Reload";

// Exposed object name for AddJavaScriptMessageHandler
const char* MSG_HANDLER_OBJ = "daliBridge";

// Test page: has selectable text and a div for JS injection
const char* TEST_HTML =
  "<!DOCTYPE html><html><head><meta charset='UTF-8'>"
  "<title>JS Integration Test</title>"
  "<style>"
  "body{background:#1a1a2e;color:#eee;font-family:sans-serif;padding:24px}"
  "h2{color:#89b4fa}p{line-height:1.6}#log{background:#0d0d1a;padding:12px;"
  "border-radius:6px;font-size:13px;min-height:80px;white-space:pre-wrap}"
  "#msgBtn{margin-top:10px;padding:8px 16px;background:#313149;color:#eee;"
  "border:none;border-radius:4px;cursor:pointer}"
  "</style></head>"
  "<body>"
  "<h2>WebView JavaScript Integration Test</h2>"
  "<p id='sel'>Select this text to test <b>GetSelectedText</b>.</p>"
  "<div id='log'>Waiting for JS evaluation...</div>"
  "<button id='msgBtn' style='display:none' onclick=\"daliBridge.postMessage('Hello from JS button')\">Send Message</button>"
  "<script>"
  "window.appendLog = function(msg) {"
  "  var el = document.getElementById('log');"
  "  el.textContent += '\\n' + msg;"
  "};"
  "</script>"
  "</body></html>";

const char* JS_EVAL_CHANGE_BG =
  "document.body.style.background='#2e1a1a';"
  "window.appendLog('[1] BG changed to dark red via EvaluateJavaScript (no callback)');";

const char* JS_EVAL_TITLE =
  "document.title";

const char* JS_EVAL_UA =
  "navigator.userAgent";

const char* JS_INJECT_MSG_BTN =
  "document.getElementById('msgBtn').style.display='inline-block';"
  "window.appendLog('[4] Message handler button injected. Click it or press 4 again.');";

const char* JS_TRIGGER_ALERT =
  "window.appendLog('[5] Triggering alert...');"
  "alert('Hello from dali-ui WebView alert test!');";

const char* JS_TRIGGER_CONFIRM =
  "window.appendLog('[6] Triggering confirm...');"
  "var r = confirm('Confirm dialog test. OK or Cancel?');"
  "window.appendLog('[6] confirm result: ' + r);";

const char* JS_TRIGGER_PROMPT =
  "window.appendLog('[7] Triggering prompt...');"
  "var r = prompt('Enter your name:', 'Dali');"
  "window.appendLog('[7] prompt result: ' + r);";
} // namespace

class JavaScriptTestController : public ConnectionTracker
{
public:
  explicit JavaScriptTestController(Application& application)
  : mApplication(application)
  {
    mApplication.InitSignal().Connect(this, &JavaScriptTestController::OnInit);
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

    // Register message handler BEFORE loading page so it is available from page start
    mWebView.AddJavaScriptMessageHandler(
      Dali::String(MSG_HANDLER_OBJ),
      WebView::JavaScriptCallback::New(this, &JavaScriptTestController::OnJsMessage));

    // Register dialog callbacks
    mWebView.RegisterJavaScriptAlertCallback(
      WebView::JavaScriptAlertCallback::New(this, &JavaScriptTestController::OnJsAlert));
    mWebView.RegisterJavaScriptConfirmCallback(
      WebView::JavaScriptConfirmCallback::New(this, &JavaScriptTestController::OnJsConfirm));
    mWebView.RegisterJavaScriptPromptCallback(
      WebView::JavaScriptPromptCallback::New(this, &JavaScriptTestController::OnJsPrompt));

    mWebView.PageLoadFinishedSignal().Connect(this, &JavaScriptTestController::OnPageLoadFinished);

    window.KeyEventSignal().Connect(this, &JavaScriptTestController::OnKeyEvent);

    mWebView.LoadHtmlString(Dali::String(TEST_HTML));
    SetInfo("Test page loaded via LoadHtmlString");
    DALI_LOG_RELEASE_INFO("[JSTest] Init complete. MessageHandler '%s' registered.\n", MSG_HANDLER_OBJ);
  }

  // ---------------------------------------------------------------------------
  // Signal handlers
  // ---------------------------------------------------------------------------

  void OnPageLoadFinished(WebView /*view*/, const Dali::String& /*url*/)
  {
    DALI_LOG_RELEASE_INFO("[JSTest] Page load finished. JS API ready.\n");
  }

  // ---------------------------------------------------------------------------
  // JS callbacks
  // ---------------------------------------------------------------------------

  void OnJsResult(const Dali::String& result)
  {
    SetInfo("JS result: %s", result.CStr());
    DALI_LOG_RELEASE_INFO("[JSTest] EvaluateJavaScript result: \"%s\"\n", result.CStr());
  }

  void OnJsMessage(const Dali::String& message)
  {
    SetInfo("JS message: %s", message.CStr());
    DALI_LOG_RELEASE_INFO("[JSTest] AddJavaScriptMessageHandler received: \"%s\"\n", message.CStr());
  }

  bool OnJsAlert(const Dali::String& message)
  {
    SetInfo("ALERT: %s", message.CStr());
    DALI_LOG_RELEASE_INFO("[JSTest] alert(\"%s\") — replying\n", message.CStr());
    mWebView.JavaScriptAlertReply();
    return true; // true = we handled it (suppress default dialog)
  }

  bool OnJsConfirm(const Dali::String& message)
  {
    SetInfo("CONFIRM: %s  => OK", message.CStr());
    DALI_LOG_RELEASE_INFO("[JSTest] confirm(\"%s\") — replying true\n", message.CStr());
    mWebView.JavaScriptConfirmReply(true);
    return true;
  }

  bool OnJsPrompt(const Dali::String& message, const Dali::String& defaultValue)
  {
    Dali::String response = Dali::String("DaliReply_") + defaultValue;
    SetInfo("PROMPT: %s  default=%s  => %s", message.CStr(), defaultValue.CStr(), response.CStr());
    DALI_LOG_RELEASE_INFO("[JSTest] prompt(\"%s\", \"%s\") — replying \"%s\"\n",
                          message.CStr(), defaultValue.CStr(), response.CStr());
    mWebView.JavaScriptPromptReply(response);
    return true;
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
      mWebView.EvaluateJavaScript(Dali::String(JS_EVAL_CHANGE_BG));
      SetInfo("EvaluateJavaScript (no cb): change background");
      DALI_LOG_RELEASE_INFO("[JSTest] EvaluateJavaScript (no callback): change BG\n");
    }
    else if(key == "2")
    {
      mWebView.EvaluateJavaScript(
        Dali::String(JS_EVAL_TITLE),
        WebView::JavaScriptCallback::New(this, &JavaScriptTestController::OnJsResult));
      SetInfo("EvaluateJavaScript: document.title — waiting...");
      DALI_LOG_RELEASE_INFO("[JSTest] EvaluateJavaScript: document.title\n");
    }
    else if(key == "3")
    {
      mWebView.EvaluateJavaScript(
        Dali::String(JS_EVAL_UA),
        WebView::JavaScriptCallback::New(this, &JavaScriptTestController::OnJsResult));
      SetInfo("EvaluateJavaScript: navigator.userAgent — waiting...");
      DALI_LOG_RELEASE_INFO("[JSTest] EvaluateJavaScript: navigator.userAgent\n");
    }
    else if(key == "4")
    {
      // Inject a button that posts a message via the registered handler object
      mWebView.EvaluateJavaScript(Dali::String(JS_INJECT_MSG_BTN));
      SetInfo("AddJavaScriptMessageHandler: '%s' — button injected", MSG_HANDLER_OBJ);
      DALI_LOG_RELEASE_INFO("[JSTest] MsgHandler '%s' — injected button\n", MSG_HANDLER_OBJ);
    }
    else if(key == "5")
    {
      mWebView.EvaluateJavaScript(Dali::String(JS_TRIGGER_ALERT));
      SetInfo("Triggering alert() ...");
      DALI_LOG_RELEASE_INFO("[JSTest] Triggering alert()\n");
    }
    else if(key == "6")
    {
      mWebView.EvaluateJavaScript(Dali::String(JS_TRIGGER_CONFIRM));
      SetInfo("Triggering confirm() ...");
      DALI_LOG_RELEASE_INFO("[JSTest] Triggering confirm()\n");
    }
    else if(key == "7")
    {
      mWebView.EvaluateJavaScript(Dali::String(JS_TRIGGER_PROMPT));
      SetInfo("Triggering prompt() ...");
      DALI_LOG_RELEASE_INFO("[JSTest] Triggering prompt()\n");
    }
    else if(key == "S" || key == "s")
    {
      Dali::String sel = mWebView.GetSelectedText();
      SetInfo("SelectedText: \"%s\"", sel.CStr());
      DALI_LOG_RELEASE_INFO("[JSTest] GetSelectedText: \"%s\"\n", sel.CStr());
    }
    else if(key == "R" || key == "r")
    {
      mWebView.LoadHtmlString(Dali::String(TEST_HTML));
      SetInfo("Test page reloaded");
      DALI_LOG_RELEASE_INFO("[JSTest] Reloaded test page\n");
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
  Application&      mApplication;
  Label             mHintLabel;
  Label             mInfoLabel;
  Dali::Ui::WebView mWebView;
};

int DALI_EXPORT_API main(int argc, char** argv)
{
  Application application = Application::New(&argc, &argv);
  UiConfig::New().Apply();
  JavaScriptTestController test(application);
  application.MainLoop();
  return 0;
}
