# WebView Component — PR Description

## Overview

`WebView`는 DALi 애플리케이션 내에서 웹 콘텐츠를 임베딩하고 상호작용할 수 있는 UI 컴포넌트입니다.  
`Dali::Ui::View` 를 상속하며, 내부적으로 Tizen WebEngine(Blink/Chromium 기반)을 래핑합니다.

---

## 변경 파일 구조

```
dali-ui-foundation/
  public-api/
    web-view.h              # 공개 API 헤더
    web-view.cpp            # 공개 API 구현
    web-view-types.h        # 열거형 및 타입 정의
    web-view.autogen.h      # 자동 생성 매크로
  integration-api/
    web-view-impl.h         # 내부 구현 인터페이스
    web-view-impl.cpp       # 내부 구현

samples/web-view/
  web-view-example.cpp      # 기본 WebView 예제
  web-browser-example.cpp   # 풀 브라우저 UI 예제

automated-tests/src/dali-ui-foundation/
  utc-Dali-WebView.cpp      # 단위 테스트
```

---

## API 목록

### 페이지 로딩

| 메서드 | 설명 |
|--------|------|
| `LoadUrl(url)` | URL을 비동기로 로드 |
| `LoadHtmlString(html)` | 인라인 HTML 문자열을 로드 |
| `LoadHtmlStringOverrideCurrentEntry(html, baseUrl, unreachableUrl)` | 커스텀 base URL과 함께 HTML 로드 |
| `LoadContents(contents, size, mimeType, encoding, baseUrl)` | MIME 타입 지정 콘텐츠 로드 |
| `Reload()` | 현재 페이지 새로고침 |
| `ReloadWithoutCache()` | 캐시 무시하고 새로고침 |
| `StopLoading()` | 페이지 로드 중단 |

### 탐색 (Navigation)

| 메서드 | 설명 |
|--------|------|
| `GoForward()` | 앞으로 이동 |
| `GoBack()` | 뒤로 이동 |
| `CanGoForward()` → bool | 앞으로 이동 가능 여부 |
| `CanGoBack()` → bool | 뒤로 이동 가능 여부 |
| `ClearHistory()` | 탐색 히스토리 전체 삭제 |

### 페이지 정보 조회

| 메서드 | 설명 |
|--------|------|
| `GetUrl()` → String | 현재 로드된 URL 반환 |
| `GetTitle()` → String | 페이지 타이틀 반환 |
| `GetLoadProgressPercentage()` → float | 로드 진행률 반환 (0.0 ~ 100.0) |
| `GetFavicon()` → ImageView | 페이지 파비콘 반환 |
| `GetSelectedText()` → String | 현재 선택된 텍스트 반환 |
| `GetPlainTextAsynchronously(callback)` | 페이지 텍스트를 비동기로 추출 |

### 사용자 에이전트 및 HTTP 헤더

| 메서드 | 설명 |
|--------|------|
| `SetUserAgent(userAgent)` | 커스텀 유저 에이전트 설정 |
| `GetUserAgent()` → String | 현재 유저 에이전트 반환 |
| `AddCustomHeader(name, value)` → bool | 요청에 커스텀 HTTP 헤더 추가 |
| `RemoveCustomHeader(name)` → bool | 커스텀 HTTP 헤더 제거 |

### JavaScript 연동

| 메서드 | 설명 |
|--------|------|
| `EvaluateJavaScript(script)` | JavaScript 코드 실행 |
| `EvaluateJavaScript(script, callback)` | JavaScript 실행 후 결과 콜백 수신 |
| `AddJavaScriptMessageHandler(objectName, callback)` | JS에서 호출 가능한 네이티브 객체 등록 |
| `RegisterJavaScriptAlertCallback(callback)` | JS `alert()` 처리 콜백 등록 |
| `RegisterJavaScriptConfirmCallback(callback)` | JS `confirm()` 처리 콜백 등록 |
| `RegisterJavaScriptPromptCallback(callback)` | JS `prompt()` 처리 콜백 등록 |
| `JavaScriptAlertReply()` | Alert 다이얼로그에 응답 |
| `JavaScriptConfirmReply(confirmed)` | Confirm 다이얼로그에 응답 |
| `JavaScriptPromptReply(result)` | Prompt 다이얼로그에 응답 |

### 스크롤

| 메서드 | 설명 |
|--------|------|
| `GetScrollPosition()` → Vector2 | 현재 스크롤 위치 반환 |
| `GetScrollSize()` → Vector2 | 스크롤 가능 영역 크기 반환 |
| `GetContentSize()` → Vector2 | 전체 콘텐츠 크기 반환 |
| `ScrollBy(deltaX, deltaY)` | 상대적 스크롤 |
| `ScrollEdgeBy(deltaX, deltaY)` → bool | 경계에 도달하면 제한되는 스크롤 |

### 줌 및 스케일

| 메서드 | 설명 |
|--------|------|
| `SetPageZoomFactor(zoom)` | 페이지 전체 줌 설정 |
| `GetPageZoomFactor()` → float | 현재 페이지 줌 반환 |
| `SetTextZoomFactor(zoom)` | 텍스트만 줌 설정 |
| `GetTextZoomFactor()` → float | 현재 텍스트 줌 반환 |
| `SetScaleFactor(scale, point)` | 특정 좌표 기준 스케일 설정 |
| `GetScaleFactor()` → float | 현재 스케일 반환 |

### 스크린샷 및 렌더링

| 메서드 | 설명 |
|--------|------|
| `GetScreenshot(viewArea, scaleFactor)` → ImageView | 동기 스크린샷 캡처 |
| `GetScreenshotAsynchronously(viewArea, scaleFactor, callback)` → bool | 비동기 스크린샷 캡처 |

### 입력 이벤트

| 메서드 | 설명 |
|--------|------|
| `SetMouseEventsEnabled(enabled)` | 마우스 이벤트 포워딩 활성화/비활성화 |
| `GetMouseEventsEnabled()` → bool | 마우스 이벤트 활성화 상태 반환 |
| `SetKeyEventsEnabled(enabled)` | 키 이벤트 포워딩 활성화/비활성화 |
| `GetKeyEventsEnabled()` → bool | 키 이벤트 활성화 상태 반환 |
| `FeedMouseWheel(yDirection, step, x, y)` | 마우스 휠 이벤트 직접 주입 |
| `FeedKeyEvent(keyEvent)` → bool | 키 이벤트 직접 주입 |
| `FeedTouchEvent(touchEvent)` → bool | 터치 이벤트 직접 주입 |

### 비디오 재생

| 메서드 | 설명 |
|--------|------|
| `SetVideoHoleEnabled(enabled)` | 비디오 홀 렌더링 활성화/비활성화 |
| `GetVideoHoleEnabled()` → bool | 비디오 홀 활성화 상태 반환 |
| `CheckVideoPlayingAsynchronously(callback)` → bool | 비디오 재생 중 여부 비동기 확인 |

### 네트워크 및 렌더링 제어

| 메서드 | 설명 |
|--------|------|
| `SuspendNetworkLoading()` | 네트워크 활동 일시 정지 |
| `ResumeNetworkLoading()` | 네트워크 활동 재개 |
| `Suspend()` | WebView 일시 정지 |
| `Resume()` | WebView 재개 |
| `SetDocumentBackgroundColor(color)` | 문서 배경색 설정 |
| `SetTilesClearedWhenHidden(cleared)` | 숨김 시 타일 캐시 정리 여부 설정 |
| `SetTileCoverAreaMultiplier(multiplier)` | 렌더링 캐시 배율 설정 |
| `SetCursorEnabledByClient(enabled)` | 클라이언트가 커서를 직접 관리하도록 설정 |
| `ClearAllTilesResources()` | 렌더링 캐시 전체 삭제 |
| `ExitFullscreen()` | 전체화면 모드 종료 |

---

## 시그널 (이벤트 콜백)

| 시그널 | 파라미터 | 설명 |
|--------|----------|------|
| `PageLoadStartedSignal` | `(WebView, String url)` | 페이지 로드 시작 시 발생 |
| `PageLoadInProgressSignal` | `(WebView, String url)` | 페이지 로드 중 발생 |
| `PageLoadFinishedSignal` | `(WebView, String url)` | 페이지 로드 완료 시 발생 |
| `UrlChangedSignal` | `(WebView, String url)` | URL 변경 시 발생 |
| `FrameRenderedSignal` | `(WebView)` | 프레임 렌더링 완료마다 발생 |
| `ScrollEdgeReachedSignal` | `(WebView, WebViewScrollEdge)` | 스크롤 경계 도달 시 발생 |
| `OverScrolledSignal` | `(WebView, WebViewOverScrolled)` | 오버스크롤 발생 시 |
| `FullscreenEnteredSignal` | `(WebView)` | 전체화면 진입 시 발생 |
| `FullscreenExitedSignal` | `(WebView)` | 전체화면 종료 시 발생 |
| `TextFoundSignal` | `(WebView, uint32_t count)` | 텍스트 검색 결과 반환 시 발생 |
| `GeolocationPermissionSignal` | `(WebView, String origin, String url)` → bool | 위치 권한 요청 시 발생 |
| `WebProcessCrashedSignal` | `(WebView)` | 웹 프로세스 크래시 시 발생 |

---

## 프로퍼티 (Dali Property System)

| 프로퍼티 | 타입 | 설명 |
|----------|------|------|
| `"url"` | STRING | 현재 로드된 URL |
| `"userAgent"` | STRING | 유저 에이전트 문자열 |
| `"scrollPosition"` | VECTOR2 | 현재 스크롤 위치 |
| `"scrollSize"` | VECTOR2 | 스크롤 가능 영역 크기 |
| `"contentSize"` | VECTOR2 | 전체 콘텐츠 크기 |
| `"title"` | STRING | 페이지 타이틀 |
| `"videoHoleEnabled"` | BOOLEAN | 비디오 홀 렌더링 활성화 여부 |
| `"mouseEventsEnabled"` | BOOLEAN | 마우스 이벤트 활성화 여부 |
| `"keyEventsEnabled"` | BOOLEAN | 키 이벤트 활성화 여부 |
| `"documentBackgroundColor"` | VECTOR4 | 문서 배경색 |
| `"tilesClearedWhenHidden"` | BOOLEAN | 숨김 시 타일 캐시 정리 여부 |
| `"tileCoverAreaMultiplier"` | FLOAT | 렌더링 캐시 배율 |
| `"cursorEnabledByClient"` | BOOLEAN | 클라이언트 커서 관리 여부 |
| `"selectedText"` | STRING | 현재 선택된 텍스트 |
| `"pageZoomFactor"` | FLOAT | 페이지 줌 배율 |
| `"textZoomFactor"` | FLOAT | 텍스트 줌 배율 |
| `"loadProgressPercentage"` | FLOAT | 로드 진행률 (0.0 ~ 100.0) |

---

## 열거형 (Enumerations)

### `WebViewScrollEdge`
스크롤 경계 방향을 나타냅니다.
- `LEFT`, `RIGHT`, `TOP`, `BOTTOM`

### `WebViewOverScrolled`
오버스크롤 방향을 나타냅니다.
- `LEFT`, `RIGHT`, `TOP`, `BOTTOM`

### `WebViewFindOption` (비트 플래그)
텍스트 검색 옵션입니다.

| 플래그 | 설명 |
|--------|------|
| `NONE` | 기본 검색 |
| `CASE_INSENSITIVE` | 대소문자 무시 |
| `AT_WORD_STARTS` | 단어 시작 부분만 검색 |
| `TREAT_MEDIAL_CAPITAL_AS_WORD_START` | 중간 대문자를 단어 시작으로 취급 |
| `BACKWARDS` | 역방향 검색 |
| `WRAP_AROUND` | 끝에서 처음으로 감싸기 |
| `SHOW_OVERLAY` | 오버레이로 결과 표시 |
| `SHOW_FIND_INDICATOR` | 검색 인디케이터 표시 |
| `SHOW_HIGHLIGHT` | 검색 결과 하이라이트 |

---

## 사용 예시

```cpp
// 기본 WebView 생성 및 URL 로드
WebView view = WebView::New();
view.LoadUrl("https://www.samsung.com");

// 페이지 로드 이벤트 구독
view.PageLoadStartedSignal().Connect(this, &Controller::OnPageLoadStarted);
view.PageLoadFinishedSignal().Connect(this, &Controller::OnPageLoadFinished);

// JavaScript 실행 및 결과 수신
view.EvaluateJavaScript("document.title", [](const std::string& result) {
    // result 처리
});

// 커스텀 HTTP 헤더 추가
view.AddCustomHeader("X-Custom-Header", "value");
```

---

## 아키텍처 및 의존성

- **Dali::WebEngine** (dali-adaptor) — 실제 웹 렌더링 엔진 (Tizen WebEngine, Blink/Chromium 기반)
- **Dali::Ui::View** — 레이아웃 및 렌더링 베이스 클래스
- 비동기 작업의 콜백은 메인 스레드에서 호출됨
- WebEngine은 별도의 샌드박스 프로세스에서 실행됨
