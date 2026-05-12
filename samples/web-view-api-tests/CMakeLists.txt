CMAKE_MINIMUM_REQUIRED(VERSION 3.8.2)
PROJECT(web-view-api-tests)
SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_C_STANDARD 99)

FIND_PACKAGE(PkgConfig REQUIRED)
PKG_CHECK_MODULES(DALICORE      REQUIRED dali2-core)
PKG_CHECK_MODULES(DALIADAPTOR   REQUIRED dali2-adaptor)
PKG_CHECK_MODULES(DALI_UI_FOUNDATION REQUIRED dali2-ui-foundation)

# ---------------------------------------------------------------------------
# Macro: add one WebView API test executable
# ---------------------------------------------------------------------------
MACRO(ADD_WEBVIEW_API_TEST EXE_NAME SOURCE_FILE)
  ADD_EXECUTABLE(${EXE_NAME} ${SOURCE_FILE})

  SET_TARGET_PROPERTIES(${EXE_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")

  INSTALL(TARGETS ${EXE_NAME} RUNTIME DESTINATION bin)

  TARGET_LINK_LIBRARIES(${EXE_NAME}
    ${DALICORE_LIBRARIES}
    ${DALIADAPTOR_LIBRARIES}
    ${DALI_UI_FOUNDATION_LIBRARIES})

  TARGET_LINK_DIRECTORIES(${EXE_NAME}
    PRIVATE $ENV{DESKTOP_PREFIX}/lib)

  TARGET_INCLUDE_DIRECTORIES(${EXE_NAME}
    PRIVATE
      ${DALICORE_INCLUDE_DIRS}
      ${DALIADAPTOR_INCLUDE_DIRS}
      ${DALI_UI_FOUNDATION_INCLUDE_DIRS})

  TARGET_COMPILE_OPTIONS(${EXE_NAME}
    PRIVATE
      ${DALICORE_CFLAGS_OTHER}
      ${DALIADAPTOR_CFLAGS_OTHER}
      ${DALI_UI_FOUNDATION_CFLAGS_OTHER})
ENDMACRO()

# ---------------------------------------------------------------------------
# Group 1: Page loading, navigation, content info, user-agent, custom headers
# APIs: LoadUrl, LoadHtmlString, LoadHtmlStringOverrideCurrentEntry, LoadContents,
#       Reload, ReloadWithoutCache, StopLoading, GoForward, GoBack,
#       CanGoForward, CanGoBack, ClearHistory, GetUrl, GetTitle,
#       GetLoadProgressPercentage, GetFavicon, GetPlainTextAsynchronously,
#       SetUserAgent, GetUserAgent, AddCustomHeader, RemoveCustomHeader
# Signals: PageLoadStarted, PageLoadInProgress, PageLoadFinished, UrlChanged
# ---------------------------------------------------------------------------
ADD_WEBVIEW_API_TEST(web-view-loading-test web-view-loading-test.cpp)

# ---------------------------------------------------------------------------
# Group 2: JavaScript integration and dialog callbacks
# APIs: EvaluateJavaScript (x2), AddJavaScriptMessageHandler,
#       RegisterJavaScriptAlertCallback, JavaScriptAlertReply,
#       RegisterJavaScriptConfirmCallback, JavaScriptConfirmReply,
#       RegisterJavaScriptPromptCallback, JavaScriptPromptReply,
#       GetSelectedText
# ---------------------------------------------------------------------------
ADD_WEBVIEW_API_TEST(web-view-javascript-test web-view-javascript-test.cpp)

# ---------------------------------------------------------------------------
# Group 3: Scroll and zoom/scale APIs
# APIs: GetScrollPosition, GetScrollSize, GetContentSize,
#       ScrollBy, ScrollEdgeBy,
#       SetPageZoomFactor, GetPageZoomFactor,
#       SetTextZoomFactor, GetTextZoomFactor,
#       SetScaleFactor, GetScaleFactor
# Signals: ScrollEdgeReachedSignal, OverScrolledSignal
# ---------------------------------------------------------------------------
ADD_WEBVIEW_API_TEST(web-view-scroll-zoom-test web-view-scroll-zoom-test.cpp)

# ---------------------------------------------------------------------------
# Group 4: Input event forwarding and screenshot APIs
# APIs: SetMouseEventsEnabled, GetMouseEventsEnabled,
#       SetKeyEventsEnabled, GetKeyEventsEnabled,
#       FeedMouseWheel, FeedKeyEvent, FeedTouchEvent,
#       GetScreenshot, GetScreenshotAsynchronously
# Signals: KeyEventSignal, TouchEventSignal
# ---------------------------------------------------------------------------
ADD_WEBVIEW_API_TEST(web-view-input-screenshot-test web-view-input-screenshot-test.cpp)

# ---------------------------------------------------------------------------
# Group 5: Lifecycle, rendering settings, network, video, and fullscreen
# APIs: Suspend, Resume, SuspendNetworkLoading, ResumeNetworkLoading,
#       SetDocumentBackgroundColor, SetTilesClearedWhenHidden,
#       SetTileCoverAreaMultiplier, SetCursorEnabledByClient,
#       ClearAllTilesResources, SetVideoHoleEnabled, GetVideoHoleEnabled,
#       CheckVideoPlayingAsynchronously, ExitFullscreen
# Signals: FrameRenderedSignal, FullscreenEnteredSignal, FullscreenExitedSignal,
#          WebProcessCrashedSignal, GeolocationPermissionSignal
# ---------------------------------------------------------------------------
ADD_WEBVIEW_API_TEST(web-view-lifecycle-test web-view-lifecycle-test.cpp)
