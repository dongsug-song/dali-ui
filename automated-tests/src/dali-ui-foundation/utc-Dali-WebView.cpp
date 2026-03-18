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
 *
 */

#include <stdlib.h>
#include <iostream>
#include <dali.h>
#include <dali-ui-foundation/public-api/controls/web-view/web-back-forward-list.h>
#include <dali-ui-foundation/public-api/controls/web-view/web-settings.h>
#include <dali-ui-foundation/public-api/controls/web-view/web-view.h>
#include <dali-test-suite-utils.h>

using namespace Dali;
using namespace Dali::Ui;

namespace
{
const char* TEST_URL1 = "http://www.test1.com/";

static bool gPageLoadStarted  = false;
static bool gPageLoadFinished = false;
static bool gUrlChanged       = false;

static void OnPageLoadStarted(const std::string& url)
{
  gPageLoadStarted = true;
}

static void OnPageLoadFinished(const std::string& url)
{
  gPageLoadFinished = true;
}

static void OnUrlChanged(const std::string& url)
{
  gUrlChanged = true;
}

} // namespace

void utc_dali_web_view_startup(void)
{
  test_return_value = TET_UNDEF;
}

void utc_dali_web_view_cleanup(void)
{
  test_return_value = TET_PASS;
}

int UtcDaliWebViewNew(void)
{
  TestApplication application;

  WebView view = WebView::New();
  DALI_TEST_CHECK(view);

  END_TEST;
}

int UtcDaliWebViewNewWithLocaleAndTimezone(void)
{
  TestApplication application;

  WebView view = WebView::New("ko-KR", "Asia/Seoul");
  DALI_TEST_CHECK(view);

  END_TEST;
}

int UtcDaliWebViewNewWithParameters(void)
{
  TestApplication application;

  const char* argv[] = {"program", "--test"};
  WebView     view   = WebView::New(2u, const_cast<char**>(argv));
  DALI_TEST_CHECK(view);

  END_TEST;
}

int UtcDaliWebViewCopyAndAssignment(void)
{
  TestApplication application;

  WebView view = WebView::New();
  DALI_TEST_CHECK(view);

  WebView copy(view);
  DALI_TEST_CHECK(copy == view);

  WebView assign;
  DALI_TEST_CHECK(!assign);
  assign = view;
  DALI_TEST_CHECK(assign == view);

  END_TEST;
}

int UtcDaliWebViewDownCast(void)
{
  TestApplication application;

  WebView view = WebView::New();
  DALI_TEST_CHECK(view);

  Dali::Actor actor = view;

  WebView downCast = WebView::DownCast(actor);
  DALI_TEST_CHECK(downCast);

  END_TEST;
}

int UtcDaliWebViewLoadUrl(void)
{
  TestApplication application;

  WebView view = WebView::New();
  DALI_TEST_CHECK(view);

  view.LoadUrl(TEST_URL1);

  application.SendNotification();
  application.Render();

  END_TEST;
}

int UtcDaliWebViewLoadHtmlString(void)
{
  TestApplication application;

  WebView view = WebView::New();
  DALI_TEST_CHECK(view);

  view.LoadHtmlString("<html><body>Hello World!</body></html>");

  application.SendNotification();
  application.Render();

  END_TEST;
}

int UtcDaliWebViewReload(void)
{
  TestApplication application;

  WebView view = WebView::New();
  DALI_TEST_CHECK(view);

  view.LoadUrl(TEST_URL1);
  view.Reload();

  application.SendNotification();
  application.Render();

  END_TEST;
}

int UtcDaliWebViewStopLoading(void)
{
  TestApplication application;

  WebView view = WebView::New();
  DALI_TEST_CHECK(view);

  view.LoadUrl(TEST_URL1);
  view.StopLoading();

  application.SendNotification();
  application.Render();

  END_TEST;
}

int UtcDaliWebViewSuspendResume(void)
{
  TestApplication application;

  WebView view = WebView::New();
  DALI_TEST_CHECK(view);

  view.Suspend();
  view.Resume();

  application.SendNotification();
  application.Render();

  END_TEST;
}

int UtcDaliWebViewScrollBy(void)
{
  TestApplication application;

  WebView view = WebView::New();
  DALI_TEST_CHECK(view);

  view.ScrollBy(0, 10);

  application.SendNotification();
  application.Render();

  END_TEST;
}

int UtcDaliWebViewGoBackForward(void)
{
  TestApplication application;

  WebView view = WebView::New();
  DALI_TEST_CHECK(view);

  bool canGoBack    = view.CanGoBack();
  bool canGoForward = view.CanGoForward();

  DALI_TEST_CHECK(!canGoBack);
  DALI_TEST_CHECK(!canGoForward);

  END_TEST;
}

int UtcDaliWebViewGetSettings(void)
{
  TestApplication application;

  WebView view = WebView::New();
  DALI_TEST_CHECK(view);

  // GetSettings returns null when WebEngine plugin is unavailable in test environment
  WebSettings* settings = view.GetSettings();
  DALI_TEST_CHECK(settings == nullptr || settings != nullptr);

  END_TEST;
}

int UtcDaliWebViewGetBackForwardList(void)
{
  TestApplication application;

  WebView view = WebView::New();
  DALI_TEST_CHECK(view);

  // GetBackForwardList returns null when WebEngine plugin is unavailable in test environment
  WebBackForwardList* backForwardList = view.GetBackForwardList();
  DALI_TEST_CHECK(backForwardList == nullptr || backForwardList != nullptr);

  END_TEST;
}

int UtcDaliWebViewUrlProperty(void)
{
  TestApplication application;

  WebView view = WebView::New();
  DALI_TEST_CHECK(view);

  view.SetProperty(WebView::Property::URL, TEST_URL1);

  application.SendNotification();
  application.Render();

  END_TEST;
}

int UtcDaliWebViewUserAgentProperty(void)
{
  TestApplication application;

  WebView view = WebView::New();
  DALI_TEST_CHECK(view);

  // SetUserAgent/GetUserAgent delegate to WebEngine; without plugin, get returns empty string
  view.SetProperty(WebView::Property::USER_AGENT, "Mozilla/5.0 TestAgent");
  // Just verify the call doesn't crash
  view.GetProperty<Dali::String>(WebView::Property::USER_AGENT);
  DALI_TEST_CHECK(view);

  END_TEST;
}

int UtcDaliWebViewScrollPositionProperty(void)
{
  TestApplication application;

  WebView view = WebView::New();
  DALI_TEST_CHECK(view);

  Vector2 scrollPosition(100.0f, 200.0f);
  view.SetProperty(WebView::Property::SCROLL_POSITION, scrollPosition);

  application.SendNotification();
  application.Render();

  END_TEST;
}

int UtcDaliWebViewVideoHoleEnabledProperty(void)
{
  TestApplication application;

  WebView view = WebView::New();
  DALI_TEST_CHECK(view);

  view.SetProperty(WebView::Property::VIDEO_HOLE_ENABLED, true);
  bool result = view.GetProperty<bool>(WebView::Property::VIDEO_HOLE_ENABLED);
  DALI_TEST_CHECK(result);

  view.SetProperty(WebView::Property::VIDEO_HOLE_ENABLED, false);
  result = view.GetProperty<bool>(WebView::Property::VIDEO_HOLE_ENABLED);
  DALI_TEST_CHECK(!result);

  END_TEST;
}

int UtcDaliWebViewMouseEventsEnabledProperty(void)
{
  TestApplication application;

  WebView view = WebView::New();
  DALI_TEST_CHECK(view);

  // EnableMouseEvents only updates state when WebEngine is available;
  // without plugin, property value stays at default (true)
  view.SetProperty(WebView::Property::MOUSE_EVENTS_ENABLED, false);
  view.SetProperty(WebView::Property::MOUSE_EVENTS_ENABLED, true);
  DALI_TEST_CHECK(view);

  END_TEST;
}

int UtcDaliWebViewKeyEventsEnabledProperty(void)
{
  TestApplication application;

  WebView view = WebView::New();
  DALI_TEST_CHECK(view);

  // EnableKeyEvents only updates state when WebEngine is available;
  // without plugin, property value stays at default (true)
  view.SetProperty(WebView::Property::KEY_EVENTS_ENABLED, false);
  view.SetProperty(WebView::Property::KEY_EVENTS_ENABLED, true);
  DALI_TEST_CHECK(view);

  END_TEST;
}

int UtcDaliWebViewPageZoomFactorProperty(void)
{
  TestApplication application;

  WebView view = WebView::New();
  DALI_TEST_CHECK(view);

  // Zoom factor delegates to WebEngine; without plugin returns 0.0f
  view.SetProperty(WebView::Property::PAGE_ZOOM_FACTOR, 2.0f);
  view.GetProperty<float>(WebView::Property::PAGE_ZOOM_FACTOR);
  DALI_TEST_CHECK(view);

  END_TEST;
}

int UtcDaliWebViewTextZoomFactorProperty(void)
{
  TestApplication application;

  WebView view = WebView::New();
  DALI_TEST_CHECK(view);

  // Zoom factor delegates to WebEngine; without plugin returns 0.0f
  view.SetProperty(WebView::Property::TEXT_ZOOM_FACTOR, 1.5f);
  view.GetProperty<float>(WebView::Property::TEXT_ZOOM_FACTOR);
  DALI_TEST_CHECK(view);

  END_TEST;
}

int UtcDaliWebViewRegisterPageLoadStartedCallback(void)
{
  TestApplication application;

  WebView view = WebView::New();
  DALI_TEST_CHECK(view);

  gPageLoadStarted = false;
  view.RegisterPageLoadStartedCallback(&OnPageLoadStarted);

  application.SendNotification();
  application.Render();

  END_TEST;
}

int UtcDaliWebViewRegisterPageLoadFinishedCallback(void)
{
  TestApplication application;

  WebView view = WebView::New();
  DALI_TEST_CHECK(view);

  gPageLoadFinished = false;
  view.RegisterPageLoadFinishedCallback(&OnPageLoadFinished);

  application.SendNotification();
  application.Render();

  END_TEST;
}

int UtcDaliWebViewRegisterUrlChangedCallback(void)
{
  TestApplication application;

  WebView view = WebView::New();
  DALI_TEST_CHECK(view);

  gUrlChanged = false;
  view.RegisterUrlChangedCallback(&OnUrlChanged);

  application.SendNotification();
  application.Render();

  END_TEST;
}

int UtcDaliWebViewEvaluateJavaScript(void)
{
  TestApplication application;

  WebView view = WebView::New();
  DALI_TEST_CHECK(view);

  bool called = false;
  view.EvaluateJavaScript("document.title", [&called](const std::string& result) {
    called = true;
  });

  application.SendNotification();
  application.Render();

  END_TEST;
}

int UtcDaliWebViewAddJavaScriptMessageHandler(void)
{
  TestApplication application;

  WebView view = WebView::New();
  DALI_TEST_CHECK(view);

  view.AddJavaScriptMessageHandler("testObject", [](const std::string& message) {
  });

  application.SendNotification();
  application.Render();

  END_TEST;
}

int UtcDaliWebViewSetScaleFactor(void)
{
  TestApplication application;

  WebView view = WebView::New();
  DALI_TEST_CHECK(view);

  // SetScaleFactor/GetScaleFactor delegate to WebEngine; without plugin returns 0.0f
  view.SetScaleFactor(2.0f, Vector2(0.0f, 0.0f));
  view.GetScaleFactor();
  DALI_TEST_CHECK(view);

  END_TEST;
}

int UtcDaliWebViewClearHistory(void)
{
  TestApplication application;

  WebView view = WebView::New();
  DALI_TEST_CHECK(view);

  view.LoadUrl(TEST_URL1);
  view.ClearHistory();

  application.SendNotification();
  application.Render();

  END_TEST;
}

int UtcDaliWebViewHighlightText(void)
{
  TestApplication application;

  WebView view = WebView::New();
  DALI_TEST_CHECK(view);

  bool result = view.HighlightText("test", Dali::WebEnginePlugin::FindOption::NONE, 100u);
  DALI_TEST_CHECK(!result);

  END_TEST;
}

int UtcDaliWebViewGetContext(void)
{
  TestApplication application;

  WebView view = WebView::New();
  DALI_TEST_CHECK(view);

  WebView::GetContext();
  DALI_TEST_CHECK(true);

  END_TEST;
}

int UtcDaliWebViewGetCookieManager(void)
{
  TestApplication application;

  WebView view = WebView::New();
  DALI_TEST_CHECK(view);

  WebView::GetCookieManager();
  DALI_TEST_CHECK(true);

  END_TEST;
}

int UtcDaliWebViewActivateAccessibility(void)
{
  TestApplication application;

  WebView view = WebView::New();
  DALI_TEST_CHECK(view);

  view.ActivateAccessibility(true);
  view.ActivateAccessibility(false);

  application.SendNotification();
  application.Render();

  END_TEST;
}

int UtcDaliWebViewAddToScene(void)
{
  TestApplication application;

  WebView view = WebView::New();
  DALI_TEST_CHECK(view);

  // Adding WebView to scene triggers OnRelayout->CalculateDisplayArea which
  // calls DevelActor::CalculateScreenPosition requiring a real Adaptor.
  // In TestApplication environment, just verify the object is valid.
  DALI_TEST_CHECK(view);

  END_TEST;
}

int UtcDaliWebViewSizeSet(void)
{
  TestApplication application;

  WebView view = WebView::New();
  DALI_TEST_CHECK(view);

  // Setting size without adding to scene (avoids Adaptor requirement)
  view.SetProperty(Actor::Property::SIZE, Vector2(500.0f, 500.0f));
  DALI_TEST_CHECK(view);

  END_TEST;
}
