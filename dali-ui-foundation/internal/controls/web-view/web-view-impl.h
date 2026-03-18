#ifndef DALI_UI_INTERNAL_WEB_VIEW_H
#define DALI_UI_INTERNAL_WEB_VIEW_H

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

// EXTERNAL INCLUDES
#include <dali/devel-api/actors/actor-devel.h>
#include <dali/devel-api/adaptor-framework/proxy-accessible.h>
#include <dali/devel-api/adaptor-framework/web-engine/web-engine.h>
#include <dali/public-api/object/property-notification.h>
#include <memory>

// INTERNAL INCLUDES
#include <dali-ui-foundation/devel-api/visual-factory/visual-base.h>
#include <dali-ui-foundation/integration-api/control-accessible.h>
#include <dali-ui-foundation/public-api/controls/control-impl.h>
#include <dali-ui-foundation/public-api/controls/image-view/image-view.h>
#include <dali-ui-foundation/public-api/controls/web-view/web-view.h>

namespace Dali
{
namespace Ui
{
class KeyEvent;
class TouchEvent;
class WebBackForwardList;
class WebSettings;
class WebView;

namespace Internal
{
class WebView : public Control
{
protected:
  WebView();

  WebView(const std::string& locale, const std::string& timezoneId);

  WebView(uint32_t argc, char** argv, int32_t type);

  virtual ~WebView();

public:
  /**
   * @copydoc Dali::Ui::WebView::New()
   */
  static Ui::WebView New();

  /**
   * @copydoc Dali::Ui::WebView::New( const std::string&, const std::string& )
   */
  static Ui::WebView New(const std::string& locale, const std::string& timezoneId);

  /**
   * @copydoc Dali::Ui::WebView::New( uint32_t, char**, int32_t )
   */
  static Ui::WebView New(uint32_t argc, char** argv, int32_t type);

  /**
   * @copydoc Dali::Ui::WebView::FindWebView()
   */
  static Ui::WebView FindWebView(Dali::WebEnginePlugin* plugin);

  /**
   * @copydoc Dali::Ui::WebView::GetContext()
   */
  static Dali::WebEngineContext* GetContext();

  /**
   * @copydoc Dali::Ui::WebView::GetCookieManager()
   */
  static Dali::WebEngineCookieManager* GetCookieManager();

  /**
   * @copydoc Dali::Ui::WebView::ChangeOrientation()
   */
  void ChangeOrientation(int orientation);

  /**
   * @copydoc Dali::Ui::WebView::GetSettings()
   */
  Dali::Ui::WebSettings* GetSettings() const;

  /**
   * @copydoc Dali::Ui::WebView::GetBackForwardList()
   */
  Dali::Ui::WebBackForwardList* GetBackForwardList() const;

  /**
   * @copydoc Dali::Ui::WebView::GetPlugin()
   */
  Dali::WebEnginePlugin* GetPlugin() const;

  /**
   * @copydoc Dali::Ui::WebView::GetFavicon()
   */
  Dali::Ui::ImageView GetFavicon() const;

  /**
   * @copydoc Dali::Ui::WebView::LoadUrl()
   */
  void LoadUrl(const std::string& url);

  /**
   * @copydoc Dali::WebEngine::LoadHTMLString()
   */
  void LoadHtmlString(const std::string& htmlString);

  /**
   * @copydoc Dali::WebEngine::LoadHtmlStringOverrideCurrentEntry()
   */
  bool LoadHtmlStringOverrideCurrentEntry(const std::string& html, const std::string& basicUri,
                                          const std::string& unreachableUrl);

  /**
   * @copydoc Dali::WebEngine::LoadContents()
   */
  bool LoadContents(const int8_t* contents, uint32_t contentSize, const std::string& mimeType,
                    const std::string& encoding, const std::string& baseUri);

  /**
   * @copydoc Dali::Ui::WebView::Reload()
   */
  void Reload();

  /**
   * @copydoc Dali::WebEngine::ReloadWithoutCache()
   */
  bool ReloadWithoutCache();

  /**
   * @copydoc Dali::Ui::WebView::StopLoading()
   */
  void StopLoading();

  /**
   * @copydoc Dali::Ui::WebView::Suspend()
   */
  void Suspend();

  /**
   * @copydoc Dali::Ui::WebView::Resume()
   */
  void Resume();

  /**
   * @copydoc Dali::WebEngine::SuspendNetworkLoading()
   */
  void SuspendNetworkLoading();

  /**
   * @copydoc Dali::WebEngine::ResumeNetworkLoading()
   */
  void ResumeNetworkLoading();

  /**
   * @copydoc Dali::WebEngine::AddCustomHeader()
   */
  bool AddCustomHeader(const std::string& name, const std::string& value);

  /**
   * @copydoc Dali::WebEngine::RemoveCustomHeader()
   */
  bool RemoveCustomHeader(const std::string& name);

  /**
   * @copydoc Dali::WebEngine::StartInspectorServer()
   */
  uint32_t StartInspectorServer(uint32_t port);

  /**
   * @copydoc Dali::WebEngine::StopInspectorServer()
   */
  bool StopInspectorServer();

  /**
   * @brief Set the style of IME.
   */
  bool SetImePositionAndAlignment(Dali::Vector2 position, int alignment);

  /**
   * @brief Set the theme name of cursor.
   */
  void SetCursorThemeName(const std::string themeName);

  /**
   * @copydoc Dali::Ui::WebView::ScrollBy()
   */
  void ScrollBy(int32_t deltaX, int32_t deltaY);

  /**
   * @copydoc Dali::WebEngine::ScrollEdgeBy()
   */
  bool ScrollEdgeBy(int32_t deltaX, int32_t deltaY);

  /**
   * @copydoc Dali::Ui::WebView::CanGoForward()
   */
  bool CanGoForward();

  /**
   * @copydoc Dali::Ui::WebView::GoForward()
   */
  void GoForward();

  /**
   * @copydoc Dali::Ui::WebView::CanGoBack()
   */
  bool CanGoBack();

  /**
   * @copydoc Dali::Ui::WebView::GoBack()
   */
  void GoBack();

  /**
   * @copydoc Dali::Ui::WebView::EvaluateJavaScript()
   */
  void EvaluateJavaScript(const std::string&                                      script,
                          Dali::WebEnginePlugin::JavaScriptMessageHandlerCallback resultHandler);

  /**
   * @copydoc Dali::Ui::WebView::AddJavaScriptMessageHandler()
   */
  void AddJavaScriptMessageHandler(const std::string&                                      exposedObjectName,
                                   Dali::WebEnginePlugin::JavaScriptMessageHandlerCallback handler);

  /**
   * @copydoc Dali::Ui::WebView::AddJavaScriptEntireMessageHandler()
   */
  void AddJavaScriptEntireMessageHandler(const std::string&                                            exposedObjectName,
                                         Dali::WebEnginePlugin::JavaScriptEntireMessageHandlerCallback handler);

  /**
   * @copydoc Dali::Ui::WebView::RegisterJavaScriptAlertCallback()
   */
  void RegisterJavaScriptAlertCallback(Dali::WebEnginePlugin::JavaScriptAlertCallback callback);

  /**
   * @copydoc Dali::Ui::WebView::JavaScriptAlertReply()
   */
  void JavaScriptAlertReply();

  /**
   * @copydoc Dali::Ui::WebView::RegisterJavaScriptConfirmCallback()
   */
  void RegisterJavaScriptConfirmCallback(Dali::WebEnginePlugin::JavaScriptConfirmCallback callback);

  /**
   * @copydoc Dali::Ui::WebView::JavaScriptConfirmReply()
   */
  void JavaScriptConfirmReply(bool confirmed);

  /**
   * @copydoc Dali::Ui::WebView::RegisterJavaScriptPromptCallback()
   */
  void RegisterJavaScriptPromptCallback(Dali::WebEnginePlugin::JavaScriptPromptCallback callback);

  /**
   * @copydoc Dali::Ui::WebView::JavaScriptPromptReply()
   */
  void JavaScriptPromptReply(const std::string& result);

  /**
   * @copydoc Dali::Ui::WebView::CreateHitTest()
   */
  std::unique_ptr<Dali::WebEngineHitTest> CreateHitTest(int32_t x, int32_t y, Dali::WebEngineHitTest::HitTestMode mode);

  /**
   * @copydoc Dali::Ui::WebView::CreateHitTestAsynchronously()
   */
  bool CreateHitTestAsynchronously(int32_t x, int32_t y, Dali::WebEngineHitTest::HitTestMode mode,
                                   Dali::WebEnginePlugin::WebEngineHitTestCreatedCallback callback);

  /**
   * @copydoc Dali::Ui::WebView::ClearHistory()
   */
  void ClearHistory();

  /**
   * @copydoc Dali::Ui::WebView::ClearAllTilesResources()
   */
  void ClearAllTilesResources();

  /**
   * @copydoc Dali::Ui::WebView::SetScaleFactor()
   */
  void SetScaleFactor(float scaleFactor, Dali::Vector2 point);

  /**
   * @copydoc Dali::Ui::WebView::GetScaleFactor()
   */
  float GetScaleFactor() const;

  /**
   * @copydoc Dali::Ui::WebView::ActivateAccessibility()
   */
  void ActivateAccessibility(bool activated);

  /**
   * @copydoc Dali::Ui::WebView::HighlightText()
   */
  bool HighlightText(const std::string& text, Dali::WebEnginePlugin::FindOption options, uint32_t maxMatchCount);

  /**
   * @copydoc Dali::Ui::WebView::AddDynamicCertificatePath()
   */
  void AddDynamicCertificatePath(const std::string& host, const std::string& certPath);

  /**
   * @copydoc Dali::Ui::WebView::GetScreenshot()
   */
  Dali::Ui::ImageView GetScreenshot(Dali::Rect<int32_t> viewArea, float scaleFactor);

  /**
   * @copydoc Dali::Ui::WebView::GetScreenshotAsynchronously()
   */
  bool GetScreenshotAsynchronously(Dali::Rect<int32_t> viewArea, float scaleFactor,
                                   Dali::Ui::WebView::WebViewScreenshotCapturedCallback callback);

  /**
   * @copydoc Dali::Ui::WebView::CheckVideoPlayingAsynchronously()
   */
  bool CheckVideoPlayingAsynchronously(Dali::WebEnginePlugin::VideoPlayingCallback callback);

  /**
   * @copydoc Dali::Ui::WebView::ExitFullscreen()
   */
  void ExitFullscreen();

  /**
   * @copydoc Dali::Ui::WebView::RegisterGeolocationPermissionCallback()
   */
  void RegisterGeolocationPermissionCallback(Dali::WebEnginePlugin::GeolocationPermissionCallback callback);

  /**
   * @copydoc Dali::Ui::WebView::SetTtsFocus()
   */
  void SetTtsFocus(bool focused);

  void RegisterPageLoadStartedCallback(Dali::WebEnginePlugin::WebEnginePageLoadCallback callback);
  void RegisterPageLoadInProgressCallback(Dali::WebEnginePlugin::WebEnginePageLoadCallback callback);
  void RegisterPageLoadFinishedCallback(Dali::WebEnginePlugin::WebEnginePageLoadCallback callback);
  void RegisterPageLoadErrorCallback(Dali::WebEnginePlugin::WebEnginePageLoadErrorCallback callback);
  void RegisterScrollEdgeReachedCallback(Dali::WebEnginePlugin::WebEngineScrollEdgeReachedCallback callback);
  void RegisterOverScrolledCallback(Dali::WebEnginePlugin::WebEngineOverScrolledCallback callback);
  void RegisterUrlChangedCallback(Dali::WebEnginePlugin::WebEngineUrlChangedCallback callback);
  void RegisterFormRepostDecidedCallback(Dali::WebEnginePlugin::WebEngineFormRepostDecidedCallback callback);
  void RegisterFrameRenderedCallback(Dali::WebEnginePlugin::WebEngineFrameRenderedCallback callback);
  void RegisterConsoleMessageReceivedCallback(Dali::WebEnginePlugin::WebEngineConsoleMessageReceivedCallback callback);
  void RegisterResponsePolicyDecidedCallback(Dali::WebEnginePlugin::WebEngineResponsePolicyDecidedCallback callback);
  void RegisterNavigationPolicyDecidedCallback(
    Dali::WebEnginePlugin::WebEngineNavigationPolicyDecidedCallback callback);
  void RegisterNewWindowPolicyDecidedCallback(Dali::WebEnginePlugin::WebEngineNewWindowPolicyDecidedCallback callback);
  void RegisterNewWindowCreatedCallback(Dali::WebEnginePlugin::WebEngineNewWindowCreatedCallback callback);
  void RegisterCertificateConfirmedCallback(Dali::WebEnginePlugin::WebEngineCertificateCallback callback);
  void RegisterSslCertificateChangedCallback(Dali::WebEnginePlugin::WebEngineCertificateCallback callback);
  void RegisterHttpAuthHandlerCallback(Dali::WebEnginePlugin::WebEngineHttpAuthHandlerCallback callback);
  void RegisterContextMenuShownCallback(Dali::WebEnginePlugin::WebEngineContextMenuShownCallback callback);
  void RegisterContextMenuHiddenCallback(Dali::WebEnginePlugin::WebEngineContextMenuHiddenCallback callback);
  void RegisterFullscreenEnteredCallback(Dali::WebEnginePlugin::WebEngineFullscreenEnteredCallback callback);
  void RegisterFullscreenExitedCallback(Dali::WebEnginePlugin::WebEngineFullscreenExitedCallback callback);
  void RegisterTextFoundCallback(Dali::WebEnginePlugin::WebEngineTextFoundCallback callback);
  void GetPlainTextAsynchronously(Dali::WebEnginePlugin::PlainTextReceivedCallback callback);
  void WebAuthenticationCancel();
  void RegisterWebAuthDisplayQRCallback(Dali::WebEnginePlugin::WebEngineWebAuthDisplayQRCallback callback);
  void RegisterWebAuthResponseCallback(Dali::WebEnginePlugin::WebEngineWebAuthResponseCallback callback);
  void RegisterFileChooserRequestedCallback(Dali::WebEnginePlugin::WebEngineFileChooserRequestedCallback callback);
  void RegisterWebProcessCrashedCallback(Dali::WebEnginePlugin::WebEngineWebProcessCrashedCallback callback);
  void RegisterUserMediaPermissionRequestCallback(
    Dali::WebEnginePlugin::WebEngineUserMediaPermissionRequestCallback callback);
  void RegisterDeviceConnectionChangedCallback(
    Dali::WebEnginePlugin::WebEngineDeviceConnectionChangedCallback callback);
  void RegisterDeviceListGetCallback(Dali::WebEnginePlugin::WebEngineDeviceListGetCallback callback);
  void FeedMouseWheel(bool yDirection, int step, int x, int y);
  void SetVideoHole(bool enabled, bool isWaylandWindow);

public: // Properties
  static void                  SetProperty(Dali::BaseObject* object, Dali::Property::Index index, const Dali::Property::Value& value);
  static Dali::Property::Value GetProperty(Dali::BaseObject* object, Dali::Property::Index propertyIndex);

private: // From Control
  void               OnInitialize() override;
  ControlAccessible* CreateAccessibleObject() override;
  void               OnRelayout(const Vector2& size, RelayoutContainer& container) override;
  Vector3            GetNaturalSize() override;
  bool               OnKeyEvent(const Dali::KeyEvent& event) override;
  void               OnKeyInputFocusGained() override;
  void               OnKeyInputFocusLost() override;
  void               OnSceneConnection(int depth) override;
  void               OnSceneDisconnection() override;

private:
  // Undefined
  WebView(const WebView& webView);
  WebView& operator=(const WebView& webView);

  void                SetScrollPosition(int32_t x, int32_t y);
  Dali::Vector2       GetScrollPosition() const;
  Dali::Vector2       GetScrollSize() const;
  Dali::Vector2       GetContentSize() const;
  std::string         GetTitle() const;
  void                SetDocumentBackgroundColor(Dali::Vector4 color);
  void                ClearTilesWhenHidden(bool cleared);
  void                SetTileCoverAreaMultiplier(float multiplier);
  void                EnableCursorByClient(bool enabled);
  std::string         GetSelectedText() const;
  std::string         GetUrl() const;
  std::string         GetUserAgent() const;
  void                SetUserAgent(const std::string& userAgent);
  void                SetPageZoomFactor(float zoomFactor);
  float               GetPageZoomFactor() const;
  void                SetTextZoomFactor(float zoomFactor);
  float               GetTextZoomFactor() const;
  float               GetLoadProgressPercentage() const;
  bool                SetVisibility(bool visible);
  void                EnableVideoHole(bool enabled);
  void                EnableBlendMode(bool blendEnabled);
  void                EnableMouseEvents(bool enabled);
  void                EnableKeyEvents(bool enabled);
  Dali::Ui::ImageView CreateImageView(Dali::PixelData pixel) const;

  bool OnTouchEvent(Actor actor, const Dali::TouchEvent& touch);
  bool OnHoverEvent(Actor actor, const Dali::HoverEvent& hover);
  bool OnWheelEvent(Actor actor, const Dali::WheelEvent& wheel);
  void OnFrameRendered();
  void OnDisplayAreaUpdated(Dali::PropertyNotification& source);
  void OnInheritedVisibilityChanged(Actor actor, bool isVisible);
  void OnScreenshotCaptured(Dali::PixelData pixel);
  void SetDisplayArea(const Dali::Rect<int32_t>& displayArea);

protected:
  class WebViewAccessible : public ControlAccessible
  {
  public:
    WebViewAccessible() = delete;

    WebViewAccessible(Dali::Actor self, Dali::WebEngine& webEngine);

  protected:
    Dali::Accessibility::Attributes GetAttributes() const override;
    void                            DoGetChildren(std::vector<Dali::Accessibility::Accessible*>& children) override;

  private:
    void OnAccessibilityEnabled();
    void OnAccessibilityDisabled();
    void SetRemoteChildAddress(Dali::Accessibility::Address address);

    Dali::Accessibility::ProxyAccessible mRemoteChild;
    Dali::WebEngine&                     mWebEngine;
  };

private:
  Dali::Ui::Visual::Base mVisual;
  Dali::Size             mWebViewSize;
  Dali::WebEngine        mWebEngine;

  uint32_t mLastRenderedNativeImageWidth;
  uint32_t mLastRenderedNativeImageHeight;

  std::unique_ptr<Dali::Ui::WebSettings>        mWebSettings;
  std::unique_ptr<Dali::Ui::WebBackForwardList> mWebBackForwardList;

  Dali::PropertyNotification mPositionUpdateNotification;
  Dali::PropertyNotification mSizeUpdateNotification;
  Dali::PropertyNotification mScaleUpdateNotification;
  Dali::Rect<int32_t>        mWebViewArea;
  bool                       mVideoHoleEnabled;
  bool                       mMouseEventsEnabled;
  bool                       mKeyEventsEnabled;

  bool mVisualChangeRequired;

  Dali::Ui::WebView::WebViewScreenshotCapturedCallback  mScreenshotCapturedCallback;
  Dali::WebEnginePlugin::WebEngineFrameRenderedCallback mFrameRenderedCallback;

  static std::unordered_map<Dali::WebEnginePlugin*, Dali::WeakHandle<Ui::WebView>> mPluginWebViewMap;
};

} // namespace Internal

inline Ui::Internal::WebView& GetImpl(Ui::WebView& handle)
{
  DALI_ASSERT_ALWAYS(handle);
  Dali::RefObject& impl = handle.GetImplementation();
  return static_cast<Ui::Internal::WebView&>(impl);
}

inline const Ui::Internal::WebView& GetImpl(const Ui::WebView& handle)
{
  DALI_ASSERT_ALWAYS(handle);
  const Dali::RefObject& impl = handle.GetImplementation();
  return static_cast<const Ui::Internal::WebView&>(impl);
}

} // namespace Ui

} // namespace Dali

#endif // DALI_UI_INTERNAL_WEB_VIEW_H
