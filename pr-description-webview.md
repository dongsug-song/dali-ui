## Summary

- Port WebView implementation from `dali-ui` to the updated `dali-ui-0319` codebase
- Add public API, internal implementation, and automated UTC tests for WebView

## Changes

### New files added
- `dali-ui-foundation/public-api/controls/web-view/web-view.h/.cpp`
- `dali-ui-foundation/public-api/controls/web-view/web-settings.h/.cpp`
- `dali-ui-foundation/public-api/controls/web-view/web-back-forward-list.h/.cpp`
- `dali-ui-foundation/internal/controls/web-view/web-view-impl.h/.cpp`
- `automated-tests/src/dali-ui-foundation/utc-Dali-WebView.cpp`

### Adaptations from previous codebase

| Item | Change |
|------|--------|
| Internal include path | `<controls/control/...>` → `<dali-ui-foundation/internal/controls/control/...>` |
| `control-depth-index-ranges.h` | moved from `devel-api/controls/` → `public-api/controls/` |
| type-registry headers | moved from `public-api/object/` → `devel-api/object/` |
| `ACCESSIBILITY_ROLE` property | `DevelControl::Property::` → `Ui::Control::Property::` (promoted to public API) |
| Visual registration | `DevelControl::RegisterVisual/UnregisterVisual/EnableCornerPropertiesOverridden` → `Control::Impl::Get(*this).RegisterVisual/UnregisterVisual/EnableCornerPropertiesOverridden` (direct Impl access, no DevelControl wrapper needed) |

## Public API — `Dali::Ui::WebView`

### Factory / Lifecycle
| Method | Description |
|--------|-------------|
| `WebView::New()` | Create a WebView |
| `WebView::New(locale, timezoneId)` | Create with locale and timezone |
| `WebView::New(argc, argv)` | Create with application arguments |
| `WebView::FindWebView(plugin)` | Find WebView by engine plugin handle |
| `WebView::GetContext()` | Get shared WebEngine context |
| `WebView::GetCookieManager()` | Get shared cookie manager |
| `WebView::DownCast(handle)` | Downcast a base handle to WebView |

### Navigation
| Method | Description |
|--------|-------------|
| `LoadUrl(url)` | Load a URL |
| `LoadHtmlString(html)` | Load raw HTML string |
| `LoadHtmlStringOverrideCurrentEntry(html, baseUri, unreachableUrl)` | Load HTML overriding history entry |
| `LoadContents(contents, size, mimeType, encoding, baseUri)` | Load raw content by MIME type |
| `Reload()` | Reload current page |
| `ReloadWithoutCache()` | Reload without cache |
| `StopLoading()` | Stop loading |
| `CanGoBack()` / `GoBack()` | Back navigation |
| `CanGoForward()` / `GoForward()` | Forward navigation |
| `ClearHistory()` | Clear navigation history |

### Display / Viewport
| Method | Description |
|--------|-------------|
| `ScrollBy(deltaX, deltaY)` | Scroll by delta |
| `ScrollEdgeBy(deltaX, deltaY)` | Scroll edge by delta |
| `SetScaleFactor(factor, point)` | Set page scale |
| `GetScaleFactor()` | Get current scale |
| `ChangeOrientation(orientation)` | Change display orientation |
| `SetVideoHole(enabled, isWayland)` | Enable video hole rendering |
| `ExitFullscreen()` | Exit fullscreen mode |

### JavaScript
| Method | Description |
|--------|-------------|
| `EvaluateJavaScript(script, callback)` | Evaluate JS with result callback |
| `EvaluateJavaScript(script)` | Evaluate JS without callback |
| `AddJavaScriptMessageHandler(name, handler)` | Inject JS object with handler |
| `AddJavaScriptEntireMessageHandler(name, handler)` | Inject JS object with full-message handler |
| `RegisterJavaScriptAlertCallback(cb)` | Handle JS alert |
| `RegisterJavaScriptConfirmCallback(cb)` | Handle JS confirm |
| `RegisterJavaScriptPromptCallback(cb)` | Handle JS prompt |
| `JavaScriptAlertReply()` / `JavaScriptConfirmReply(bool)` / `JavaScriptPromptReply(result)` | Reply to JS dialogs |

### Accessibility / Input
| Method | Description |
|--------|-------------|
| `ActivateAccessibility(activated)` | Enable/disable accessibility |
| `SetTtsFocus(focused)` | Set TTS focus |
| `HighlightText(text, options, maxCount)` | Find and highlight text |
| `SetImePositionAndAlignment(pos, align)` | Set IME style |
| `SetCursorThemeName(name)` | Set cursor theme |
| `FeedMouseWheel(yDir, step, x, y)` | Feed wheel event |

### Screenshots / Hit-test
| Method | Description |
|--------|-------------|
| `GetScreenshot(area, scale)` | Capture screenshot synchronously |
| `GetScreenshotAsynchronously(area, scale, cb)` | Capture screenshot asynchronously |
| `CreateHitTest(x, y, mode)` | Create hit test synchronously |
| `CreateHitTestAsynchronously(x, y, mode, cb)` | Create hit test asynchronously |
| `CheckVideoPlayingAsynchronously(cb)` | Check if video is playing |

### Network / Headers / Security
| Method | Description |
|--------|-------------|
| `AddCustomHeader(name, value)` | Add custom HTTP header |
| `RemoveCustomHeader(name)` | Remove custom HTTP header |
| `SuspendNetworkLoading()` / `ResumeNetworkLoading()` | Pause/resume network |
| `StartInspectorServer(port)` / `StopInspectorServer()` | DevTools inspector |
| `AddDynamicCertificatePath(host, certPath)` | Set per-host client certificate |
| `RegisterCertificateConfirmedCallback(cb)` | Handle certificate confirmation |
| `RegisterSslCertificateChangedCallback(cb)` | Handle SSL certificate change |
| `RegisterHttpAuthHandlerCallback(cb)` | Handle HTTP auth |
| `WebAuthenticationCancel()` | Cancel WebAuthn |
| `RegisterWebAuthDisplayQRCallback(cb)` | Handle WebAuthn QR display |
| `RegisterWebAuthResponseCallback(cb)` | Handle WebAuthn response |

### Event Callbacks
| Method | Description |
|--------|-------------|
| `RegisterPageLoadStartedCallback(cb)` | Page load started |
| `RegisterPageLoadInProgressCallback(cb)` | Page load in progress |
| `RegisterPageLoadFinishedCallback(cb)` | Page load finished |
| `RegisterPageLoadErrorCallback(cb)` | Page load error |
| `RegisterScrollEdgeReachedCallback(cb)` | Scroll edge reached |
| `RegisterOverScrolledCallback(cb)` | Over-scrolled |
| `RegisterUrlChangedCallback(cb)` | URL changed |
| `RegisterFormRepostDecidedCallback(cb)` | Form repost decision |
| `RegisterFrameRenderedCallback(cb)` | Frame rendered |
| `RegisterConsoleMessageReceivedCallback(cb)` | JS console message |
| `RegisterResponsePolicyDecidedCallback(cb)` | Response policy decision |
| `RegisterNavigationPolicyDecidedCallback(cb)` | Navigation policy decision |
| `RegisterNewWindowPolicyDecidedCallback(cb)` | New window policy |
| `RegisterNewWindowCreatedCallback(cb)` | New window created |
| `RegisterContextMenuShownCallback(cb)` | Context menu shown |
| `RegisterContextMenuHiddenCallback(cb)` | Context menu hidden |
| `RegisterFullscreenEnteredCallback(cb)` | Fullscreen entered |
| `RegisterFullscreenExitedCallback(cb)` | Fullscreen exited |
| `RegisterTextFoundCallback(cb)` | Text found |
| `RegisterGeolocationPermissionCallback(cb)` | Geolocation permission |
| `RegisterFileChooserRequestedCallback(cb)` | File chooser |
| `RegisterWebProcessCrashedCallback(cb)` | Web process crash |
| `RegisterUserMediaPermissionRequestCallback(cb)` | UserMedia permission |
| `RegisterDeviceConnectionChangedCallback(cb)` | Device connection change |
| `RegisterDeviceListGetCallback(cb)` | Device list get |

### Properties (`WebView::Property`)
| Property | Type | R/W | Description |
|----------|------|-----|-------------|
| `URL` | STRING | R/W | Current URL |
| `USER_AGENT` | STRING | R/W | User agent string |
| `SCROLL_POSITION` | VECTOR2 | R/W | Scroll position |
| `SCROLL_SIZE` | VECTOR2 | R | Scroll size |
| `CONTENT_SIZE` | VECTOR2 | R | Content size |
| `TITLE` | STRING | R | Page title |
| `VIDEO_HOLE_ENABLED` | BOOLEAN | R/W | Video hole |
| `MOUSE_EVENTS_ENABLED` | BOOLEAN | R/W | Mouse events |
| `KEY_EVENTS_ENABLED` | BOOLEAN | R/W | Key events |
| `DOCUMENT_BACKGROUND_COLOR` | VECTOR4 | R/W | Background color |
| `TILES_CLEARED_WHEN_HIDDEN` | BOOLEAN | R/W | Tile clearing |
| `TILE_COVER_AREA_MULTIPLIER` | FLOAT | R/W | Tile cover ratio |
| `CURSOR_ENABLED_BY_CLIENT` | BOOLEAN | R/W | Client cursor |
| `SELECTED_TEXT` | STRING | R | Selected text |
| `PAGE_ZOOM_FACTOR` | FLOAT | R/W | Page zoom |
| `TEXT_ZOOM_FACTOR` | FLOAT | R/W | Text zoom |
| `LOAD_PROGRESS_PERCENTAGE` | FLOAT | R | Load progress |

## Test plan

- [x] Main library build succeeds (`make install -j9`)
- [x] UTC test build succeeds (`bash build.sh dali-ui-foundation`)
- [x] All 35 WebView UTC tests pass (`bash execute.sh -p UtcDaliWebView`)
