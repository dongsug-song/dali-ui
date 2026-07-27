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

// 이 샘플은 wiki/Accessibility-(kr).md의 내용을 실제 코드와 연결해서 읽을 수 있도록 만든다.
// 각 섹션의 주석에는 API의 의미뿐 아니라 실타겟에서 Screen Reader로 확인하는 방법도 함께 적었다.
// 시작 시 실행되는 자체검사는 setter/getter와 데이터 조작 API를 확인하고, 8개 화면의 수동 검사는
// AT-SPI bridge, Screen Reader highlight, 동적 property 조회와 action 역방향 전달을 확인한다.

#include <dali-ui-foundation/dali-ui-foundation.h>
#include <dali-ui-foundation/extension-api/view.h>
#include <dali-ui-foundation/public-api/configuration/ui-localization-manager.h>
#include <dali-ui-foundation/public-api/layouts/stack-layout.h>
#include <dali-ui-foundation/public-api/views/text-controls/label.h>
#include <dali-ui-foundation/public-api/views/view-impl.h>
#include <dali/devel-api/atspi-interfaces/accessible.h>
#include <dali/devel-api/object/type-registry-helper.h>
#include <dali/integration-api/adaptor-framework/accessibility/accessibility-bridge.h>
#include <dali/integration-api/adaptor-framework/accessibility/accessibility-service.h>
#include <dali/integration-api/debug.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace
{
using namespace Dali;
using namespace Dali::Ui;

namespace AdaptorAccessibility = Dali::Integration::Accessibility;
namespace UiAccessibility      = Dali::Ui::Accessibility;

constexpr float PAGE_HEIGHT    = 398.0f;
constexpr float CONTROL_HEIGHT = 56.0f;
constexpr int   PAGE_COUNT     = 8;

// ViewImpl의 action virtual을 하나의 callback으로 전달하기 위한 내부 구분값이다.
// wiki의 Custom View 표에 있는 action 계열 virtual 6개와 정확히 대응한다.
enum class VirtualRequest
{
  ACTIVATE,
  ESCAPE,
  VALUE_INCREASE,
  VALUE_DECREASE,
  SCROLL_TO_CHILD,
  PAN,
  ZOOM
};

// 동적 문자열 virtual 3개를 구분한다.
enum class TextRequest
{
  NAME,
  DESCRIPTION,
  VALUE
};

// 화면의 접근성 버튼이 실행할 샘플 명령이다.
enum class Command
{
  NEXT_PAGE,
  NEXT_ROLE,
  NEXT_BEHAVIOR,
  NEXT_STATE,
  NEXT_READING_INFO,
  TOGGLE_RELATIONS,
  TOGGLE_TRANSLATION,
  REINDEX_COLLECTION,
  GRAB_HIGHLIGHT,
  CLEAR_HIGHLIGHT,
  TRIGGER_READING_STATUS,
  RUN_VIRTUAL_FALLBACKS,
  CYCLE_RAW_ATTRIBUTES
};

const char* ToString(UiAccessibility::ReadingStatus status)
{
  switch(status)
  {
    case UiAccessibility::ReadingStatus::SKIPPED:
      return "SKIPPED";
    case UiAccessibility::ReadingStatus::PAUSED:
      return "PAUSED";
    case UiAccessibility::ReadingStatus::RESUMED:
      return "RESUMED";
    case UiAccessibility::ReadingStatus::CANCELLED:
      return "CANCELLED";
    case UiAccessibility::ReadingStatus::STOPPED:
      return "STOPPED";
  }
  return "UNKNOWN";
}

void Log(const std::string& message)
{
  DALI_LOG_RELEASE_INFO("[A11Y_WIKI_TARGET] %s\n", message.c_str());
  std::cout << "[A11Y_WIKI_TARGET] " << message << std::endl;
}

Label MakeLabel(const char* text, float fontSize, uint32_t color, float height, bool multiLine = false)
{
  Label label = Label::New(text);
  label.SetRequestedWidth(MATCH_PARENT);
  label.SetRequestedHeight(height);
  label.SetFontSize(fontSize);
  label.SetTextColor(UiColor(color));
  label.SetMultiLine(multiLine);
  return label;
}

bool Equals(Dali::StringView value, const char* text)
{
  const std::string expected(text);
  return value.Size() == expected.size() && std::string(value.Data(), value.Size()) == expected;
}

// 번역 resource API를 실제 번역 파일 없이도 타겟에서 검증하기 위한 샘플 번역 함수다.
// UiLocalizationManager가 resource id와 domain을 전달하면 여기서 번역 결과를 반환한다.
bool LocalizeAccessibilityText(Dali::StringView resourceId, Dali::StringView domain, Dali::String& result)
{
  if(Equals(resourceId, "IDS_WIKI_WIFI_NAME"))
  {
    result = Equals(domain, "wiki-sample") ? "와이파이 설정" : "기본 도메인 와이파이";
    return true;
  }
  if(Equals(resourceId, "IDS_WIKI_WIFI_DESCRIPTION"))
  {
    result = "사용 가능한 네트워크를 엽니다";
    return true;
  }
  return false;
}

// 일반 View handle만으로는 OnAccessibility... virtual을 override할 수 없다.
// 따라서 wiki의 설명처럼 ViewImpl을 상속한 구현 객체를 만들고, bridge에서 들어온 요청을
// 샘플 controller의 callback으로 전달한다.
class WikiAccessibilityViewImpl : public ViewImpl
{
public:
  using Ptr            = Dali::IntrusivePtr<WikiAccessibilityViewImpl>;
  using ActionCallback = std::function<bool(VirtualRequest, View)>;
  using TextCallback   = std::function<bool(TextRequest, Dali::String&)>;

  static Ptr New(ActionCallback actionCallback = {}, TextCallback textCallback = {})
  {
    return Ptr(new WikiAccessibilityViewImpl(std::move(actionCallback), std::move(textCallback)));
  }

  // [실타겟 테스트]
  // Screen Reader highlight를 이 View에 둔 뒤 double tap한다.
  // AT-SPI Action.DoAction 요청이 "activate" action으로 변환되어 이 virtual에 도달해야 한다.
  bool OnAccessibilityActivate() override
  {
    return DispatchAction(VirtualRequest::ACTIVATE);
  }

  // [실타겟 테스트]
  // Screen Reader가 escape action을 지원하면 뒤로가기 gesture를 사용한다.
  // 프로파일이 escape action을 보내지 않는 경우 7번 화면의 보조 테스트 버튼으로 같은 dispatch를 확인한다.
  bool OnAccessibilityEscape() override
  {
    return DispatchAction(VirtualRequest::ESCAPE);
  }

  // [실타겟 테스트]
  // ADJUSTABLE role의 View에서 Screen Reader의 값 증가/감소 gesture를 수행한다.
  // isIncreased가 true이면 증가, false이면 감소 요청이다.
  bool OnAccessibilityValueChange(bool isIncreased) override
  {
    return DispatchAction(isIncreased ? VirtualRequest::VALUE_INCREASE : VirtualRequest::VALUE_DECREASE);
  }

  // [실타겟 테스트]
  // scrollable로 설정한 이 View의 child에 Screen Reader highlight를 이동한다.
  // DALi가 child를 화면에 보이게 만들기 위해 parent에 이 callback을 호출한다.
  bool OnAccessibilityScrollToChild(View child) override
  {
    return mActionCallback && mActionCallback(VirtualRequest::SCROLL_TO_CHILD, child);
  }

  // 접근성 pan/zoom의 실제 gesture 제공 여부는 타겟 Screen Reader 프로파일마다 다르다.
  // 지원 프로파일에서는 gesture가 bridge를 통해 도달하고, 미지원 프로파일에서는 7번 화면의
  // 보조 테스트 버튼이 타겟 프로세스 안에서 virtual override 자체를 실행해 결과를 남긴다.
  bool OnAccessibilityPan(PanGesture) override
  {
    return DispatchAction(VirtualRequest::PAN);
  }

  bool OnAccessibilityZoom() override
  {
    return DispatchAction(VirtualRequest::ZOOM);
  }

  // [실타겟 테스트]
  // Screen Reader가 View를 읽을 때 name/description/value를 AT-SPI로 조회한다.
  // true를 반환하면 여기서 채운 동적 문자열이 사용되고, false이면 SetAccessibility... 저장값으로 fallback한다.
  bool OnAccessibilityRequestName(Dali::String& value) override
  {
    return DispatchText(TextRequest::NAME, value);
  }

  bool OnAccessibilityRequestDescription(Dali::String& value) override
  {
    return DispatchText(TextRequest::DESCRIPTION, value);
  }

  bool OnAccessibilityRequestValue(Dali::String& value) override
  {
    return DispatchText(TextRequest::VALUE, value);
  }

protected:
  ~WikiAccessibilityViewImpl() override = default;

private:
  WikiAccessibilityViewImpl(ActionCallback actionCallback, TextCallback textCallback)
  : mActionCallback(std::move(actionCallback)),
    mTextCallback(std::move(textCallback))
  {
  }

  bool DispatchAction(VirtualRequest request)
  {
    return mActionCallback && mActionCallback(request, View{});
  }

  bool DispatchText(TextRequest request, Dali::String& value)
  {
    return mTextCallback && mTextCallback(request, value);
  }

  ActionCallback mActionCallback;
  TextCallback   mTextCallback;
};

View CreateWikiAccessibilityView(WikiAccessibilityViewImpl** implementation = nullptr,
                                 WikiAccessibilityViewImpl::ActionCallback actionCallback = {},
                                 WikiAccessibilityViewImpl::TextCallback textCallback = {})
{
  auto impl = WikiAccessibilityViewImpl::New(std::move(actionCallback), std::move(textCallback));
  if(implementation)
  {
    *implementation = impl.Get();
  }
  View view(*impl);
  impl->Initialize();
  return view;
}

Dali::BaseHandle CreateRegisteredWikiAccessibilityView()
{
  return CreateWikiAccessibilityView();
}

DALI_TYPE_REGISTRATION_BEGIN(WikiAccessibilityViewImpl, Dali::Ui::ViewImpl, CreateRegisteredWikiAccessibilityView)
DALI_TYPE_REGISTRATION_END()

class AccessibilityWikiTargetTest : public ConnectionTracker
{
public:
  explicit AccessibilityWikiTargetTest(Application& application)
  : mApplication(application)
  {
    mApplication.InitSignal().Connect(this, &AccessibilityWikiTargetTest::OnInit);
  }

private:
  void OnInit(Application application)
  {
    auto localization = UiLocalizationManager::Get();
    localization.SetLocalizedStringOverride(LocalizeAccessibilityText);

    Window window = application.GetWindow();
    window.SetBackgroundColor(Color::WHITE);
    window.KeyEventSignal().Connect(this, &AccessibilityWikiTargetTest::OnKeyEvent);

    StackLayout root = StackLayout::New(StackOrientation::VERTICAL);
    root.SetRequestedWidth(MATCH_PARENT);
    root.SetRequestedHeight(MATCH_PARENT);
    root.SetSpacing(5.0f);
    root.SetPadding(Extents(16, 16, 12, 12));
    root.SetAccessibilityRole(UiAccessibility::Role::CONTAINER);
    root.SetAccessibilityHighlightable(false);

    mTitle = MakeLabel("DALi Accessibility wiki 전체 API 테스트", 21.0f, 0x17202Au, 32.0f);
    mTitle.SetAccessibilityRole(UiAccessibility::Role::HEADER);
    mTitle.SetAccessibilityName("DALi Accessibility wiki 전체 API 테스트");

    // SetRequestInitialAccessibilityHighlight(true)는 View가 accessibility tree에 들어가기 전에 설정한다.
    // [실타겟 테스트] Screen Reader를 먼저 켜고 앱을 실행했을 때 이 제목이 초기 대상으로 선택되는지 확인한다.
    mTitle.SetRequestInitialAccessibilityHighlight(true);
    root.Add(mTitle);

    mBridgeStatus = MakeLabel("AT-SPI bridge 확인 중", 14.0f, 0x34495Eu, 38.0f, true);
    mBridgeStatus.SetAccessibilityRole(UiAccessibility::Role::NOTIFICATION);
    root.Add(mBridgeStatus);

    mPageGuide = MakeLabel("페이지 준비 중", 14.0f, 0x5D6D7Eu, 68.0f, true);
    mPageGuide.SetAccessibilityRole(UiAccessibility::Role::TEXT);
    root.Add(mPageGuide);

    mPageHost = View::New();
    mPageHost.SetRequestedWidth(MATCH_PARENT);
    mPageHost.SetRequestedHeight(PAGE_HEIGHT);
    mPageHost.SetAccessibilityRole(UiAccessibility::Role::CONTAINER);
    mPageHost.SetAccessibilityHighlightable(false);
    root.Add(mPageHost);

    BuildPages();

    // 모든 페이지에서 접근할 수 있는 다음 페이지 버튼이다.
    // Screen Reader double tap으로 페이지를 바꾸므로 activate action 경로도 반복해서 검증된다.
    mNextPage = AddActionCard(root,
                              Command::NEXT_PAGE,
                              "다음 테스트 페이지",
                              "double tap하면 다음 wiki API 테스트 페이지로 이동합니다",
                              "다음 페이지 (1/8)",
                              52.0f,
                              0xD6EAF8u,
                              &mNextPageLabel);

    mOverall = MakeLabel("전체 결과 계산 중", 14.0f, 0xA04000u, 50.0f, true);
    mOverall.SetAccessibilityRole(UiAccessibility::Role::NOTIFICATION);
    root.Add(mOverall);

    RunAllApiSelfChecks();
    UpdateSelfCheckReport();
    ShowPage(0);

    // 숨김 API의 실타겟 검증용 sentinel이다. 화면에는 1픽셀 공간을 갖지만 접근성 tree에서는 숨겨진다.
    // Screen Reader가 이 객체의 동적 name을 조회하면 숨김 filtering 실패로 기록한다.
    mHiddenSentinel = CreateWikiAccessibilityView(
      nullptr,
      {},
      [this](TextRequest, Dali::String& value) {
        mHiddenSentinelExposed = true;
        value                  = "실패, accessibility hidden 객체가 노출되었습니다";
        Log("RESULT=FAIL hidden sentinel property query");
        RefreshOverall();
        return true;
      });
    mHiddenSentinel.SetRequestedWidth(MATCH_PARENT);
    mHiddenSentinel.SetRequestedHeight(1.0f);
    mHiddenSentinel.SetAccessibilityRole(UiAccessibility::Role::NOTIFICATION);
    mHiddenSentinel.SetAccessibilityName("실패, 숨김 객체 노출");
    mHiddenSentinel.SetAccessibilityHidden(true);
    root.Add(mHiddenSentinel);

    window.Add(root);

    AdaptorAccessibility::Bridge::EnabledSignal().Connect(this, &AccessibilityWikiTargetTest::OnBridgeChanged);
    AdaptorAccessibility::Bridge::DisabledSignal().Connect(this, &AccessibilityWikiTargetTest::OnBridgeChanged);
    AdaptorAccessibility::Bridge::ScreenReaderEnabledSignal().Connect(this, &AccessibilityWikiTargetTest::OnBridgeChanged);
    AdaptorAccessibility::Bridge::ScreenReaderDisabledSignal().Connect(this, &AccessibilityWikiTargetTest::OnBridgeChanged);

    RefreshOverall("application initialized");
  }

  // 새 페이지는 항상 화면에 붙여 두되, 현재 페이지가 아니면 visual visibility와 accessibility hidden을
  // 동시에 변경한다. wiki에서 강조한 것처럼 시각 상태와 접근성 노출 상태를 일관되게 관리하기 위함이다.
  StackLayout NewPage()
  {
    StackLayout page = StackLayout::New(StackOrientation::VERTICAL);
    page.SetRequestedWidth(MATCH_PARENT);
    page.SetRequestedHeight(MATCH_PARENT);
    page.SetSpacing(4.0f);
    page.SetAccessibilityRole(UiAccessibility::Role::CONTAINER);
    page.SetAccessibilityHighlightable(false);
    page.SetVisible(false);
    page.SetAccessibilityHidden(true);
    mPageHost.Add(page);
    mPages.push_back(page);
    return page;
  }

  View AddActionCard(StackLayout& parent,
                     Command command,
                     const char* name,
                     const char* description,
                     const char* visibleText,
                     float height = CONTROL_HEIGHT,
                     uint32_t color = 0xEAF2F8u,
                     Label* outputLabel = nullptr)
  {
    View control = CreateWikiAccessibilityView(
      nullptr,
      [this, command](VirtualRequest request, View) {
        if(request != VirtualRequest::ACTIVATE)
        {
          return false;
        }
        return HandleCommand(command);
      });
    control.SetRequestedWidth(MATCH_PARENT);
    control.SetRequestedHeight(height);
    control.SetBackgroundColor(UiColor(color));
    control.SetAccessibilityRole(UiAccessibility::Role::BUTTON);
    control.SetAccessibilityName(name);
    control.SetAccessibilityDescription(description);
    control.SetFocusable(true);
    control.AddAccessibilityState(UiAccessibility::State::ENABLED);
    control.AddAccessibilityReadingInfo(UiAccessibility::ReadingInfo::NAME);
    control.AddAccessibilityReadingInfo(UiAccessibility::ReadingInfo::ROLE);
    control.AddAccessibilityReadingInfo(UiAccessibility::ReadingInfo::DESCRIPTION);

    Label visual = MakeLabel(visibleText, 14.0f, 0x17202Au, height, true);
    visual.SetRequestedX(12.0f);
    visual.SetAccessibilityHidden(true);
    control.Add(visual);
    if(outputLabel)
    {
      *outputLabel = visual;
    }

    ObserveAccessibilitySignals(control, name);
    parent.Add(control);
    return control;
  }

  View AddInformationCard(StackLayout& parent,
                          UiAccessibility::Role role,
                          const char* name,
                          const char* description,
                          const char* visibleText,
                          float height,
                          uint32_t color,
                          Label* outputLabel = nullptr)
  {
    View view = View::New();
    view.SetRequestedWidth(MATCH_PARENT);
    view.SetRequestedHeight(height);
    view.SetBackgroundColor(UiColor(color));
    view.SetAccessibilityRole(role);
    view.SetAccessibilityName(name);
    view.SetAccessibilityDescription(description);
    view.AddAccessibilityState(UiAccessibility::State::ENABLED);
    view.AddAccessibilityReadingInfo(UiAccessibility::ReadingInfo::NAME);
    view.AddAccessibilityReadingInfo(UiAccessibility::ReadingInfo::ROLE);
    view.AddAccessibilityReadingInfo(UiAccessibility::ReadingInfo::DESCRIPTION);
    view.AddAccessibilityReadingInfo(UiAccessibility::ReadingInfo::STATE);

    Label visual = MakeLabel(visibleText, 14.0f, 0x17202Au, height, true);
    visual.SetRequestedX(12.0f);
    visual.SetAccessibilityHidden(true);
    view.Add(visual);
    if(outputLabel)
    {
      *outputLabel = visual;
    }
    ObserveAccessibilitySignals(view, name);
    parent.Add(view);
    return view;
  }

  void BuildPages()
  {
    BuildOverviewPage();
    BuildSemanticsPage();
    BuildReadingAndStatePage();
    BuildRelationAndLanguagePage();
    BuildCollectionPage();
    BuildHighlightAndSignalPage();
    BuildCustomVirtualPage();
    BuildRawAttributePage();
  }

  void BuildOverviewPage()
  {
    StackLayout page = NewPage();

    // 1번 화면은 앱 시작 시 자동으로 수행한 API 자체검사 결과를 보여 준다.
    // 자체검사는 타겟 CPU에서 실제 DALi 객체를 생성해 setter/getter와 export data를 확인한다.
    mSelfCheckReport = MakeLabel("API 자체검사 실행 중", 13.0f, 0x17202Au, 330.0f, true);
    mSelfCheckReport.SetAccessibilityRole(UiAccessibility::Role::TEXT);
    mSelfCheckReport.SetAccessibilityName("Accessibility wiki API 자체검사 결과");
    page.Add(mSelfCheckReport);
  }

  void BuildSemanticsPage()
  {
    StackLayout page = NewPage();

    // [wiki: 기본 정보 설정]
    // name은 대상의 짧은 이름, description은 보충 설명, value는 현재 값, role은 기능의 의미다.
    // [실타겟 테스트] 이 카드를 탐색하여 Screen Reader가 세 문자열과 role을 읽는지 확인한다.
    mSemanticTarget = AddInformationCard(page,
                                         UiAccessibility::Role::BUTTON,
                                         "의미 정보 테스트",
                                         "name description value role API를 확인합니다",
                                         "기본 정보 target: BUTTON / value=준비됨",
                                         62.0f,
                                         0xD6EAF8u,
                                         &mSemanticLabel);
    mSemanticTarget.SetAccessibilityValue("준비됨");
    mSemanticTarget.SetAutomationId("wiki-semantics-target");

    // 이 버튼은 전체 Role enum을 순서대로 SetAccessibilityRole()에 넣는다.
    // [실타겟 테스트] 버튼을 double tap한 뒤 위 target을 다시 읽어 role 발화가 바뀌는지 확인한다.
    AddActionCard(page,
                  Command::NEXT_ROLE,
                  "다음 accessibility role",
                  "semantic target의 role을 다음 enum 값으로 변경합니다",
                  "Role API: double tap하여 전체 role 순환",
                  58.0f,
                  0xFCF3CFu);

    // [wiki: 접근성 동작 속성]
    // hidden/highlightable/scrollable/modal/automation id는 서로 다른 목적을 갖는다.
    mBehaviorTarget = AddInformationCard(page,
                                         UiAccessibility::Role::BUTTON,
                                         "동작 속성 테스트 target",
                                         "hidden highlightable scrollable modal 속성을 단계별로 바꿉니다",
                                         "동작 속성: 기본 상태",
                                         62.0f,
                                         0xD5F5E3u,
                                         &mBehaviorLabel);
    mBehaviorTarget.SetAutomationId("wiki-behavior-target");

    // [실타겟 테스트]
    // 1) highlightable=false 단계에서는 target이 일반 탐색에서 건너뛰어지는지 확인한다.
    // 2) hidden=true 단계에서는 화면에는 보이지만 Screen Reader가 완전히 건너뛰는지 확인한다.
    // 3) 복원 단계에서는 role 기반 highlightable 상태로 돌아오는지 확인한다.
    AddActionCard(page,
                  Command::NEXT_BEHAVIOR,
                  "다음 접근성 동작 속성",
                  "동작 속성 target의 hidden highlightable scrollable modal을 변경합니다",
                  "동작 속성 API 단계 변경",
                  58.0f,
                  0xFADBD8u);
  }

  void BuildReadingAndStatePage()
  {
    StackLayout page = NewPage();

    // [wiki: State 관리]
    // ENABLED/SELECTED/CHECKED/BUSY/EXPANDED는 앱이 소유하는 의미 state다.
    // FOCUSED/SHOWING/HIGHLIGHTED 같은 runtime state는 이 API로 직접 추가하지 않는다.
    // [실타겟 테스트] target을 double tap하고 다시 읽어 선택됨/체크됨/바쁨/펼쳐짐 발화를 확인한다.
    mStateTarget = AddActionCard(page,
                                 Command::NEXT_STATE,
                                 "접근성 state 테스트",
                                 "double tap할 때마다 SELECTED CHECKED BUSY EXPANDED state가 바뀝니다",
                                 "State API: ENABLED",
                                 76.0f,
                                 0xD5F5E3u,
                                 &mStateLabel);
    mStateTarget.SetAccessibilityRole(UiAccessibility::Role::CHECK_BOX);
    mStateTarget.AddAccessibilityReadingInfo(UiAccessibility::ReadingInfo::STATE);

    // [wiki: 발화할 정보 선택]
    // ReadingInfo는 bit mask가 아니므로 NAME/ROLE/DESCRIPTION/STATE를 항목별로 추가·삭제한다.
    mReadingTarget = AddInformationCard(page,
                                        UiAccessibility::Role::NOTIFICATION,
                                        "ReadingInfo target 이름",
                                        "ReadingInfo description 설명",
                                        "ReadingInfo target: NAME ROLE DESCRIPTION STATE",
                                        76.0f,
                                        0xD6EAF8u,
                                        &mReadingLabel);
    mReadingTarget.SetAccessibilityValue("ReadingInfo value");

    // [실타겟 테스트] 단계 변경 후 target을 다시 탐색하여 description이나 state가 발화에서 빠지는지 확인한다.
    AddActionCard(page,
                  Command::NEXT_READING_INFO,
                  "ReadingInfo 구성 변경",
                  "description 제거, 전체 clear, 전체 복원을 순서대로 실행합니다",
                  "ReadingInfo Add / Remove / Clear / Has 테스트",
                  68.0f,
                  0xFCF3CFu);
  }

  void BuildRelationAndLanguagePage()
  {
    StackLayout page = NewPage();

    // [wiki: View 사이의 관계]
    // relation은 한 방향씩 저장되므로 LABEL_FOR/LABELLED_BY처럼 양쪽을 각각 추가한다.
    mRelationTitle = AddInformationCard(page,
                                        UiAccessibility::Role::TEXT,
                                        "비밀번호 제목",
                                        "아래 입력 target의 label입니다",
                                        "Relation source: 비밀번호",
                                        42.0f,
                                        0xEBF5FBu);
    mRelationInput = AddInformationCard(page,
                                        UiAccessibility::Role::ENTRY,
                                        "비밀번호 입력",
                                        "8자 이상 입력해야 합니다",
                                        "Relation target: password entry",
                                        42.0f,
                                        0xD6EAF8u);
    mRelationError = AddInformationCard(page,
                                        UiAccessibility::Role::ALERT,
                                        "입력 오류",
                                        "8자 이상 입력하세요",
                                        "Relation error: 8자 이상 입력",
                                        42.0f,
                                        0xFADBD8u);
    RestorePrimaryRelations();

    AddActionCard(page,
                  Command::TOGGLE_RELATIONS,
                  "relation 제거와 복원",
                  "Remove Clear Has AddAccessibilityRelation을 순서대로 실행합니다",
                  "Relation API 제거 / clear / 복원",
                  50.0f,
                  0xFCF3CFu);

    // [wiki: 다국어 접근성 문자열]
    // 첫 target은 resource binding을, 두 번째 target은 code-point 단위 language span을 사용한다.
    // [실타겟 테스트] 두 target을 읽어 한국어 번역과 Hello/세계의 언어 전환이 적용되는지 확인한다.
    mTranslationTarget = AddInformationCard(page,
                                             UiAccessibility::Role::BUTTON,
                                             "번역 binding 준비 중",
                                             "번역 description 준비 중",
                                             "Translation binding target",
                                             52.0f,
                                             0xD5F5E3u,
                                             &mTranslationLabel);
    ApplyTranslationBindings();

    mLanguageTarget = AddInformationCard(page,
                                          UiAccessibility::Role::TEXT,
                                          "Hello 세계",
                                          "English 한국어",
                                          "Language span target: Hello 세계",
                                          52.0f,
                                          0xE8DAEFu,
                                          &mLanguageLabel);
    ApplyLanguageSpans();

    AddActionCard(page,
                  Command::TOGGLE_TRANSLATION,
                  "번역과 language span clear 복원",
                  "Clear API와 Set Add API를 번갈아 실행합니다",
                  "Translation / LanguageSpan clear 및 복원",
                  52.0f,
                  0xFCF3CFu);
  }

  void BuildCollectionPage()
  {
    StackLayout page = NewPage();

    // [wiki: Collection 정보]
    // container에는 SetAccessibilityCollectionContainer(true), item에는 zero-based index를 설정한다.
    // [실타겟 테스트] 목록 항목을 순서대로 탐색하고 Screen Reader가 위치 정보를 제공하는지 확인한다.
    mCollection = StackLayout::New(StackOrientation::VERTICAL);
    mCollection.SetRequestedWidth(MATCH_PARENT);
    mCollection.SetRequestedHeight(192.0f);
    mCollection.SetSpacing(3.0f);
    mCollection.SetAccessibilityRole(UiAccessibility::Role::LIST);
    mCollection.SetAccessibilityName("샘플 collection");
    mCollection.SetAccessibilityHighlightable(false);
    mCollection.SetAccessibilityCollectionContainer(true);
    page.Add(mCollection);

    for(int index = 0; index < 3; ++index)
    {
      std::ostringstream name;
      name << "collection 항목 " << (index + 1);
      View item = AddInformationCard(mCollection,
                                     UiAccessibility::Role::LIST_ITEM,
                                     name.str().c_str(),
                                     "zero-based collection index를 가진 항목입니다",
                                     name.str().c_str(),
                                     58.0f,
                                     0xEBF5FBu);
      item.SetAccessibilityCollectionIndex(index);
      mCollectionItems.push_back(item);
    }

    // [실타겟 테스트] 역순 index, 첫 항목 clear(-1), Set(-1), 정상 순서 복원을 수행한다.
    AddActionCard(page,
                  Command::REINDEX_COLLECTION,
                  "collection index 변경",
                  "index 역순, clear, Set -1, 정상 복원을 실행합니다",
                  "Collection index 단계 변경",
                  62.0f,
                  0xFCF3CFu);

    mCollectionStatus = MakeLabel("Collection: container=true, index=0/1/2", 13.0f, 0x34495Eu, 66.0f, true);
    mCollectionStatus.SetAccessibilityRole(UiAccessibility::Role::NOTIFICATION);
    page.Add(mCollectionStatus);
  }

  void BuildHighlightAndSignalPage()
  {
    StackLayout page = NewPage();

    // [wiki: 실행 중 강제 highlight 이동]
    // 이 버튼의 activate callback에서 안정적으로 표시 중인 아래 target에 GrabAccessibilityHighlight()를 호출한다.
    // [실타겟 테스트] double tap 후 highlight가 target으로 이동하고 target 이름이 발화되는지 확인한다.
    AddActionCard(page,
                  Command::GRAB_HIGHLIGHT,
                  "highlight target으로 이동",
                  "Extension View GrabAccessibilityHighlight를 실행합니다",
                  "GrabAccessibilityHighlight 실행",
                  62.0f,
                  0xD6EAF8u);

    // target 자체를 double tap하면 ClearAccessibilityHighlight(target)를 호출한다.
    // 이때 target이 실제 highlighted 상태이므로 true가 반환되고 HighlightedSignal(false)이 발생해야 한다.
    mHighlightTarget = AddActionCard(page,
                                     Command::CLEAR_HIGHLIGHT,
                                     "highlight clear target",
                                     "현재 highlighted 상태에서 double tap하면 highlight를 clear합니다",
                                     "Target: double tap하여 ClearAccessibilityHighlight",
                                     68.0f,
                                     0xD5F5E3u);

    // ReadingStatusChangedSignal은 애플리케이션이 직접 Emit하지 않는다.
    // 이 버튼은 View type에 등록된 legacy reading action을 실행하여 signal dispatch 경로를 확인한다.
    // 실제 Screen Reader가 lifecycle action을 보내는 타겟에서는 동일 signal에 실제 이벤트도 추가로 들어온다.
    AddActionCard(page,
                  Command::TRIGGER_READING_STATUS,
                  "읽기 lifecycle signal 테스트",
                  "SKIPPED PAUSED RESUMED CANCELLED STOPPED action을 순서대로 전달합니다",
                  "ReadingStatusChangedSignal 5개 상태 실행",
                  68.0f,
                  0xFCF3CFu);

    mHighlightStatus = MakeLabel("highlight와 reading signal 대기 중", 13.0f, 0x34495Eu, 92.0f, true);
    mHighlightStatus.SetAccessibilityRole(UiAccessibility::Role::NOTIFICATION);
    page.Add(mHighlightStatus);
  }

  void BuildCustomVirtualPage()
  {
    StackLayout page = NewPage();

    // [wiki: Custom View 구현]
    // 이 target은 static 문자열도 설정하지만 동적 request virtual이 true를 반환하므로 동적 값이 우선한다.
    // [실타겟 테스트]
    // 1) target을 탐색하여 name/description/value query count가 증가하는지 확인한다.
    // 2) double tap으로 OnAccessibilityActivate(), 값 gesture로 OnAccessibilityValueChange()를 확인한다.
    mVirtualTarget = CreateWikiAccessibilityView(
      &mVirtualImpl,
      [this](VirtualRequest request, View child) {
        return HandleVirtualRequest(request, child);
      },
      [this](TextRequest request, Dali::String& value) {
        return HandleVirtualTextRequest(request, value);
      });
    mVirtualTarget.SetRequestedWidth(MATCH_PARENT);
    mVirtualTarget.SetRequestedHeight(72.0f);
    mVirtualTarget.SetBackgroundColor(UiColor(0xD6EAF8u));
    mVirtualTarget.SetAccessibilityRole(UiAccessibility::Role::ADJUSTABLE);
    mVirtualTarget.SetAccessibilityName("static fallback name");
    mVirtualTarget.SetAccessibilityDescription("static fallback description");
    mVirtualTarget.SetAccessibilityValue("static fallback value");
    mVirtualTarget.AddAccessibilityReadingInfo(UiAccessibility::ReadingInfo::NAME);
    mVirtualTarget.AddAccessibilityReadingInfo(UiAccessibility::ReadingInfo::ROLE);
    mVirtualTarget.AddAccessibilityReadingInfo(UiAccessibility::ReadingInfo::DESCRIPTION);
    mVirtualTarget.AddAccessibilityReadingInfo(UiAccessibility::ReadingInfo::STATE);
    mVirtualLabel = MakeLabel("Custom virtual target: 값 5", 14.0f, 0x17202Au, 72.0f, true);
    mVirtualLabel.SetRequestedX(12.0f);
    mVirtualLabel.SetAccessibilityHidden(true);
    mVirtualTarget.Add(mVirtualLabel);
    ObserveAccessibilitySignals(mVirtualTarget, "custom virtual target");
    page.Add(mVirtualTarget);

    // scrollable parent와 child를 실제 Actor 계층으로 만든다.
    // child가 highlight되면 ViewAccessible::ScrollToSelf()가 parent의 OnAccessibilityScrollToChild()를 호출한다.
    mScrollContainer = CreateWikiAccessibilityView(
      &mScrollContainerImpl,
      [this](VirtualRequest request, View child) {
        if(request == VirtualRequest::SCROLL_TO_CHILD)
        {
          ++mVirtualScrollCount;
          Log("OnAccessibilityScrollToChild callback");
          UpdateVirtualStatus();
          RefreshOverall();
          return child == mScrollChild;
        }
        return false;
      });
    mScrollContainer.SetRequestedWidth(MATCH_PARENT);
    mScrollContainer.SetRequestedHeight(72.0f);
    mScrollContainer.SetAccessibilityRole(UiAccessibility::Role::CONTAINER);
    mScrollContainer.SetAccessibilityName("scrollable virtual parent");
    mScrollContainer.SetAccessibilityScrollable(true);
    mScrollContainer.SetAccessibilityHighlightable(false);
    page.Add(mScrollContainer);

    StackLayout childHost = StackLayout::New(StackOrientation::VERTICAL);
    childHost.SetRequestedWidth(MATCH_PARENT);
    childHost.SetRequestedHeight(72.0f);
    childHost.SetAccessibilityRole(UiAccessibility::Role::CONTAINER);
    childHost.SetAccessibilityHighlightable(false);
    mScrollContainer.Add(childHost);
    mScrollChild = AddInformationCard(childHost,
                                      UiAccessibility::Role::BUTTON,
                                      "scroll container child",
                                      "이 child를 highlight하면 parent의 scroll callback이 호출됩니다",
                                      "Scroll child: 이 항목으로 탐색",
                                      68.0f,
                                      0xD5F5E3u);

    // escape/pan/zoom은 Screen Reader 프로파일에 따라 사용자 gesture가 없을 수 있다.
    // 이 보조 버튼은 타겟 프로세스에서 등록 action과 override를 호출하여 최소한의 실행 가능성을 보장한다.
    AddActionCard(page,
                  Command::RUN_VIRTUAL_FALLBACKS,
                  "escape pan zoom 보조 테스트",
                  "escape action과 pan zoom virtual override를 실행합니다",
                  "Virtual fallback: escape / pan / zoom / scroll",
                  60.0f,
                  0xFCF3CFu);

    mVirtualStatus = MakeLabel("Custom virtual callback 대기 중", 12.0f, 0x34495Eu, 108.0f, true);
    mVirtualStatus.SetAccessibilityRole(UiAccessibility::Role::NOTIFICATION);
    page.Add(mVirtualStatus);
  }

  void BuildRawAttributePage()
  {
    StackLayout page = NewPage();

    // [wiki: Raw attribute(원시 속성)]
    // raw attribute는 정형 API가 없는 backend 확장 정보에만 사용한다.
    // Screen Reader가 vendor key를 발화하지 않을 수 있으므로 Accessible export map을 타겟 프로세스에서 조회한다.
    mRawProbe = AddInformationCard(page,
                                   UiAccessibility::Role::BUTTON,
                                   "raw attribute probe",
                                   "Append Remove ClearAccessibilityAttributes를 확인합니다",
                                   "Raw attribute export target",
                                   70.0f,
                                   0xE8DAEFu);

    // ClearAccessibilityAttributes()는 raw key뿐 아니라 initial highlight, collection, reading info,
    // language span을 담는 typed attribute도 함께 지운다. 이 버튼이 단계별 export 결과를 화면에 표시한다.
    AddActionCard(page,
                  Command::CYCLE_RAW_ATTRIBUTES,
                  "raw attribute 단계 실행",
                  "append remove typed metadata clear restore를 순서대로 실행합니다",
                  "Raw Attribute: Append / Remove / Clear",
                  64.0f,
                  0xFCF3CFu);

    mRawStatus = MakeLabel("raw attribute 테스트 대기 중", 13.0f, 0x34495Eu, 230.0f, true);
    mRawStatus.SetAccessibilityRole(UiAccessibility::Role::NOTIFICATION);
    page.Add(mRawStatus);
  }

  void ObserveAccessibilitySignals(View view, const char* debugName)
  {
    // AccessibilityHighlightedSignal은 상태 관찰용이다. wiki 설명대로 애플리케이션에서 Emit하지 않는다.
    view.AccessibilityHighlightedSignal().Connect(this, [this, debugName](View, bool highlighted) {
      if(highlighted)
      {
        ++mHighlightTrueCount;
      }
      else
      {
        ++mHighlightFalseCount;
      }
      std::ostringstream log;
      log << "HighlightedSignal " << debugName << '=' << (highlighted ? "true" : "false");
      Log(log.str());
      UpdateHighlightStatus();
      RefreshOverall();
    });

    view.AccessibilityReadingStatusChangedSignal().Connect(
      this,
      [this, debugName](View, UiAccessibility::ReadingStatus status) {
        const std::size_t index = static_cast<std::size_t>(status);
        if(index < mReadingStatusSeen.size())
        {
          mReadingStatusSeen[index] = true;
        }
        std::ostringstream log;
        log << "ReadingStatusChangedSignal " << debugName << '=' << ToString(status);
        Log(log.str());
        UpdateHighlightStatus();
        RefreshOverall();
      });
  }

  bool HandleCommand(Command command)
  {
    switch(command)
    {
      case Command::NEXT_PAGE:
        ShowPage((mCurrentPage + 1) % PAGE_COUNT);
        return true;
      case Command::NEXT_ROLE:
        CycleRole();
        return true;
      case Command::NEXT_BEHAVIOR:
        CycleBehavior();
        return true;
      case Command::NEXT_STATE:
        CycleState();
        return true;
      case Command::NEXT_READING_INFO:
        CycleReadingInfo();
        return true;
      case Command::TOGGLE_RELATIONS:
        CycleRelations();
        return true;
      case Command::TOGGLE_TRANSLATION:
        CycleTranslation();
        return true;
      case Command::REINDEX_COLLECTION:
        CycleCollectionIndices();
        return true;
      case Command::GRAB_HIGHLIGHT:
        mGrabHighlightSucceeded = Extension::View::GrabAccessibilityHighlight(mHighlightTarget);
        Log(mGrabHighlightSucceeded ? "GrabAccessibilityHighlight=true" : "GrabAccessibilityHighlight=false");
        UpdateHighlightStatus();
        RefreshOverall();
        return mGrabHighlightSucceeded;
      case Command::CLEAR_HIGHLIGHT:
        mClearHighlightSucceeded = Extension::View::ClearAccessibilityHighlight(mHighlightTarget);
        Log(mClearHighlightSucceeded ? "ClearAccessibilityHighlight=true" : "ClearAccessibilityHighlight=false");
        UpdateHighlightStatus();
        RefreshOverall();
        return mClearHighlightSucceeded;
      case Command::TRIGGER_READING_STATUS:
        TriggerReadingStatuses();
        return true;
      case Command::RUN_VIRTUAL_FALLBACKS:
        RunVirtualFallbacks();
        return true;
      case Command::CYCLE_RAW_ATTRIBUTES:
        CycleRawAttributes();
        return true;
    }
    return false;
  }

  void ShowPage(int pageIndex)
  {
    mCurrentPage = pageIndex;
    for(int index = 0; index < static_cast<int>(mPages.size()); ++index)
    {
      const bool active = index == pageIndex;
      mPages[index].SetVisible(active);
      mPages[index].SetAccessibilityHidden(!active);
    }

    static const char* guides[PAGE_COUNT] = {
      "1/8 자체검사: 시작 시 실행된 setter/getter 및 export 결과를 확인합니다.",
      "2/8 기본/동작: target을 읽고 Role·Hidden·Highlightable·Scrollable·Modal 단계를 실행합니다.",
      "3/8 Reading/State: target을 double tap하고 발화 정보와 의미 state 변경을 확인합니다.",
      "4/8 관계/다국어: relation과 번역 binding, code-point language span을 확인합니다.",
      "5/8 Collection: 세 항목을 탐색하고 index 변경에 따른 위치 정보를 확인합니다.",
      "6/8 Highlight/Signal: Grab, Clear, highlight signal과 읽기 lifecycle signal을 확인합니다.",
      "7/8 Custom View: 동적 문자열, activate/value/escape/scroll/pan/zoom virtual을 확인합니다.",
      "8/8 Raw Attribute: raw export map과 ClearAccessibilityAttributes의 범위를 확인합니다."};
    mPageGuide.SetText(guides[pageIndex]);
    mPageGuide.SetAccessibilityName(guides[pageIndex]);

    std::ostringstream next;
    next << "다음 페이지 (" << (pageIndex + 1) << "/8)";
    mNextPageLabel.SetText(next.str().c_str());
    mNextPage.SetAccessibilityValue(next.str().c_str());

    Log(std::string("show page: ") + guides[pageIndex]);
  }

  void CycleRole()
  {
    // Accessibility::Role 전체 유효값을 순환한다. NONE도 명시적으로 highlightable=true를 설정해
    // 테스트 도중 target을 잃지 않도록 하고, role 자체의 변환은 시작 시 자체검사에서 별도로 확인한다.
    static const UiAccessibility::Role roles[] = {
      UiAccessibility::Role::ADJUSTABLE, UiAccessibility::Role::ALERT, UiAccessibility::Role::BUTTON,
      UiAccessibility::Role::CHECK_BOX, UiAccessibility::Role::COMBO_BOX, UiAccessibility::Role::CONTAINER,
      UiAccessibility::Role::DIALOG, UiAccessibility::Role::ENTRY, UiAccessibility::Role::HEADER,
      UiAccessibility::Role::IMAGE, UiAccessibility::Role::LINK, UiAccessibility::Role::LIST,
      UiAccessibility::Role::LIST_ITEM, UiAccessibility::Role::MENU, UiAccessibility::Role::MENU_BAR,
      UiAccessibility::Role::MENU_ITEM, UiAccessibility::Role::NONE, UiAccessibility::Role::NOTIFICATION,
      UiAccessibility::Role::PASSWORD_TEXT, UiAccessibility::Role::POPUP_MENU, UiAccessibility::Role::PROGRESS_BAR,
      UiAccessibility::Role::RADIO_BUTTON, UiAccessibility::Role::SCROLL_BAR, UiAccessibility::Role::SPIN_BUTTON,
      UiAccessibility::Role::TAB, UiAccessibility::Role::TAB_LIST, UiAccessibility::Role::TEXT,
      UiAccessibility::Role::TOGGLE_BUTTON, UiAccessibility::Role::TOOL_BAR, UiAccessibility::Role::SCENE_3D,
      UiAccessibility::Role::MODEL};

    mRoleIndex = (mRoleIndex + 1) % static_cast<int>(std::size(roles));
    mSemanticTarget.SetAccessibilityRole(roles[mRoleIndex]);
    mSemanticTarget.SetAccessibilityHighlightable(true);
    std::ostringstream text;
    text << "기본 정보 target: role enum index " << mRoleIndex << " / value=준비됨";
    mSemanticLabel.SetText(text.str().c_str());
    mSemanticTarget.SetAccessibilityName(text.str().c_str());
    Log(text.str());
  }

  void CycleBehavior()
  {
    mBehaviorStep = (mBehaviorStep + 1) % 6;

    // 매 단계 시작 시 모든 속성을 정상 상태로 돌려 각 단계가 독립적으로 보이도록 한다.
    mBehaviorTarget.SetAccessibilityHidden(false);
    mBehaviorTarget.ResetAccessibilityHighlightable();
    mBehaviorTarget.SetAccessibilityScrollable(false);
    mBehaviorTarget.SetAccessibilityModal(false);

    const char* status = "role 기반 기본 highlightable";
    switch(mBehaviorStep)
    {
      case 1:
        mBehaviorTarget.SetAccessibilityHighlightable(false);
        status = "highlightable=false, 일반 탐색에서 건너뜀";
        break;
      case 2:
        mBehaviorTarget.SetAccessibilityHighlightable(true);
        status = "highlightable=true 명시적 override";
        break;
      case 3:
        mBehaviorTarget.SetAccessibilityScrollable(true);
        status = "scrollable=true";
        break;
      case 4:
        mBehaviorTarget.SetAccessibilityModal(true);
        status = "modal=true";
        break;
      case 5:
        mBehaviorTarget.SetAccessibilityHidden(true);
        status = "hidden=true, 화면에는 보이고 접근성에서는 숨김";
        break;
      default:
        break;
    }
    mBehaviorLabel.SetText(status);
    Log(std::string("behavior step: ") + status);
  }

  void CycleState()
  {
    mStateStep = (mStateStep + 1) % 6;
    mStateTarget.ClearAccessibilityStates();

    const char* stateName = "ClearAccessibilityStates";
    if(mStateStep != 0)
    {
      mStateTarget.AddAccessibilityState(UiAccessibility::State::ENABLED);
    }
    switch(mStateStep)
    {
      case 1:
        stateName = "ENABLED";
        break;
      case 2:
        mStateTarget.AddAccessibilityState(UiAccessibility::State::SELECTED);
        stateName = "SELECTED";
        break;
      case 3:
        mStateTarget.AddAccessibilityState(UiAccessibility::State::CHECKED);
        stateName = "CHECKED";
        break;
      case 4:
        mStateTarget.AddAccessibilityState(UiAccessibility::State::BUSY);
        stateName = "BUSY";
        break;
      case 5:
        mStateTarget.AddAccessibilityState(UiAccessibility::State::EXPANDED);
        stateName = "EXPANDED";
        break;
      default:
        break;
    }
    std::string text = std::string("State API: ") + stateName;
    mStateLabel.SetText(text.c_str());
    mStateTarget.SetAccessibilityValue(stateName);
    Log(text);
  }

  void CycleReadingInfo()
  {
    mReadingStep = (mReadingStep + 1) % 4;
    const char* status;
    switch(mReadingStep)
    {
      case 1:
        mReadingTarget.RemoveAccessibilityReadingInfo(UiAccessibility::ReadingInfo::DESCRIPTION);
        status = "DESCRIPTION 제거";
        break;
      case 2:
        mReadingTarget.ClearAccessibilityReadingInfo();
        status = "전체 Clear";
        break;
      default:
        mReadingTarget.ClearAccessibilityReadingInfo();
        mReadingTarget.AddAccessibilityReadingInfo(UiAccessibility::ReadingInfo::NAME);
        mReadingTarget.AddAccessibilityReadingInfo(UiAccessibility::ReadingInfo::ROLE);
        mReadingTarget.AddAccessibilityReadingInfo(UiAccessibility::ReadingInfo::DESCRIPTION);
        mReadingTarget.AddAccessibilityReadingInfo(UiAccessibility::ReadingInfo::STATE);
        status = "NAME ROLE DESCRIPTION STATE 복원";
        break;
    }
    mReadingLabel.SetText(status);
    Log(std::string("ReadingInfo: ") + status);
  }

  void RestorePrimaryRelations()
  {
    mRelationTitle.AddAccessibilityRelation(UiAccessibility::RelationType::LABEL_FOR, mRelationInput);
    mRelationInput.AddAccessibilityRelation(UiAccessibility::RelationType::LABELLED_BY, mRelationTitle);
    mRelationError.AddAccessibilityRelation(UiAccessibility::RelationType::ERROR_MESSAGE, mRelationInput);
    mRelationInput.AddAccessibilityRelation(UiAccessibility::RelationType::ERROR_FOR, mRelationError);
  }

  void CycleRelations()
  {
    mRelationStep = (mRelationStep + 1) % 3;
    if(mRelationStep == 1)
    {
      mRelationInput.RemoveAccessibilityRelation(UiAccessibility::RelationType::LABELLED_BY, mRelationTitle);
      Log("RemoveAccessibilityRelation LABELLED_BY");
    }
    else if(mRelationStep == 2)
    {
      mRelationTitle.ClearAccessibilityRelations();
      mRelationInput.ClearAccessibilityRelations();
      mRelationError.ClearAccessibilityRelations();
      Log("ClearAccessibilityRelations");
    }
    else
    {
      RestorePrimaryRelations();
      Log("AddAccessibilityRelation restore");
    }
  }

  void ApplyTranslationBindings()
  {
    mTranslationTarget.SetTranslatableAccessibilityName("IDS_WIKI_WIFI_NAME", "wiki-sample");
    mTranslationTarget.SetTranslatableAccessibilityDescription("IDS_WIKI_WIFI_DESCRIPTION", "wiki-sample");
    UiLocalizationManager::Get().RefreshBindings();
    mTranslationLabel.SetText("Translation binding: 와이파이 설정");
  }

  void ApplyLanguageSpans()
  {
    mLanguageTarget.SetAccessibilityName("Hello 세계");
    mLanguageTarget.SetAccessibilityDescription("English 한국어");
    mLanguageTarget.AddAccessibilityNameLanguageSpan(0u, 5u, "en-US");
    mLanguageTarget.AddAccessibilityNameLanguageSpan(6u, 2u, "ko-KR");
    mLanguageTarget.AddAccessibilityDescriptionLanguageSpan(0u, 7u, "en-US");
    mLanguageTarget.AddAccessibilityDescriptionLanguageSpan(8u, 3u, "ko-KR");
    mLanguageLabel.SetText("Language span: Hello[en-US] 세계[ko-KR]");
  }

  void CycleTranslation()
  {
    mTranslationEnabled = !mTranslationEnabled;
    if(mTranslationEnabled)
    {
      ApplyTranslationBindings();
      ApplyLanguageSpans();
      Log("translation binding and language spans restored");
    }
    else
    {
      mTranslationTarget.ClearTranslatableAccessibilityName();
      mTranslationTarget.ClearTranslatableAccessibilityDescription();
      mTranslationTarget.SetAccessibilityName("번역 binding 해제");
      mTranslationTarget.SetAccessibilityDescription("명시적 fallback 문자열");
      mLanguageTarget.ClearAccessibilityNameLanguageSpans();
      mLanguageTarget.ClearAccessibilityDescriptionLanguageSpans();
      mTranslationLabel.SetText("Translation binding: clear");
      mLanguageLabel.SetText("Language span: clear");
      Log("ClearTranslatableAccessibility and ClearLanguageSpans");
    }
  }

  void CycleCollectionIndices()
  {
    mCollectionStep = (mCollectionStep + 1) % 4;
    std::string status;
    if(mCollectionStep == 1)
    {
      mCollectionItems[0].SetAccessibilityCollectionIndex(2);
      mCollectionItems[1].SetAccessibilityCollectionIndex(1);
      mCollectionItems[2].SetAccessibilityCollectionIndex(0);
      status = "Set index 역순: 2/1/0";
    }
    else if(mCollectionStep == 2)
    {
      mCollectionItems[0].ClearAccessibilityCollectionIndex();
      status = "첫 항목 ClearAccessibilityCollectionIndex: -1/1/0";
    }
    else if(mCollectionStep == 3)
    {
      mCollectionItems[1].SetAccessibilityCollectionIndex(-1);
      status = "두 번째 SetAccessibilityCollectionIndex(-1)";
    }
    else
    {
      for(int index = 0; index < 3; ++index)
      {
        mCollectionItems[index].SetAccessibilityCollectionIndex(index);
      }
      status = "정상 index 복원: 0/1/2";
    }
    mCollectionStatus.SetText(status.c_str());
    mCollectionStatus.SetAccessibilityName(status.c_str());
    Log(status);
  }

  void TriggerReadingStatuses()
  {
    // signal 자체의 Emit()을 호출하지 않고 View type action을 사용한다.
    // 이는 bridge가 legacy reading action을 전달할 때 사용하는 것과 동일한 View dispatch 경로다.
    Property::Map attributes;
    mHighlightTarget.DoAction("ReadingSkipped", attributes);
    mHighlightTarget.DoAction("ReadingPaused", attributes);
    mHighlightTarget.DoAction("ReadingResumed", attributes);
    mHighlightTarget.DoAction("ReadingCancelled", attributes);
    mHighlightTarget.DoAction("ReadingStopped", attributes);
    UpdateHighlightStatus();
  }

  bool HandleVirtualRequest(VirtualRequest request, View)
  {
    switch(request)
    {
      case VirtualRequest::ACTIVATE:
        ++mVirtualActivateCount;
        break;
      case VirtualRequest::ESCAPE:
        ++mVirtualEscapeCount;
        break;
      case VirtualRequest::VALUE_INCREASE:
        ++mVirtualValueCount;
        mVirtualValue = std::min(10, mVirtualValue + 1);
        break;
      case VirtualRequest::VALUE_DECREASE:
        ++mVirtualValueCount;
        mVirtualValue = std::max(0, mVirtualValue - 1);
        break;
      case VirtualRequest::PAN:
        ++mVirtualPanCount;
        break;
      case VirtualRequest::ZOOM:
        ++mVirtualZoomCount;
        break;
      case VirtualRequest::SCROLL_TO_CHILD:
        ++mVirtualScrollCount;
        break;
    }
    std::ostringstream label;
    label << "Custom virtual target: 값 " << mVirtualValue << ", activate " << mVirtualActivateCount
          << ", value callback " << mVirtualValueCount;
    mVirtualLabel.SetText(label.str().c_str());
    Log(label.str());
    UpdateVirtualStatus();
    RefreshOverall();
    return true;
  }

  bool HandleVirtualTextRequest(TextRequest request, Dali::String& value)
  {
    switch(request)
    {
      case TextRequest::NAME:
        ++mVirtualNameQueryCount;
        value = "동적 accessibility 이름";
        break;
      case TextRequest::DESCRIPTION:
        ++mVirtualDescriptionQueryCount;
        value = "ViewImpl virtual에서 제공하는 동적 설명";
        break;
      case TextRequest::VALUE:
      {
        ++mVirtualValueQueryCount;
        std::ostringstream text;
        text << mVirtualValue << " / 10";
        value = text.str().c_str();
        break;
      }
    }
    UpdateVirtualStatus();
    return true;
  }

  void RunVirtualFallbacks()
  {
    Property::Map attributes;
    mVirtualTarget.DoAction("escape", attributes);
    if(mVirtualImpl)
    {
      mVirtualImpl->OnAccessibilityPan(PanGesture{});
      mVirtualImpl->OnAccessibilityZoom();
    }
    if(mScrollContainerImpl && mVirtualScrollCount == 0)
    {
      mScrollContainerImpl->OnAccessibilityScrollToChild(mScrollChild);
    }
    UpdateVirtualStatus();
    RefreshOverall();
  }

  bool HasRawAttribute(View view, const char* key, const char* expectedValue = nullptr)
  {
    auto* accessible = Dali::Accessibility::Accessible::Get(view);
    if(!accessible)
    {
      return false;
    }
    auto attributes = accessible->GetAttributes();
    auto found      = attributes.find(key);
    return found != attributes.end() && (!expectedValue || found->second == expectedValue);
  }

  void CycleRawAttributes()
  {
    mRawStep = (mRawStep + 1) % 4;
    std::ostringstream status;
    if(mRawStep == 1)
    {
      mRawProbe.AppendAccessibilityAttribute("vendor-key", "vendor-value");
      status << "Append: " << (HasRawAttribute(mRawProbe, "vendor-key", "vendor-value") ? "PASS" : "FAIL");
    }
    else if(mRawStep == 2)
    {
      mRawProbe.RemoveAccessibilityAttribute("vendor-key");
      status << "Remove: " << (!HasRawAttribute(mRawProbe, "vendor-key") ? "PASS" : "FAIL");
    }
    else if(mRawStep == 3)
    {
      mRawProbe.AppendAccessibilityAttribute("vendor-key", "vendor-value");
      mRawProbe.SetRequestInitialAccessibilityHighlight(true);
      mRawProbe.SetAccessibilityCollectionContainer(true);
      mRawProbe.SetAccessibilityCollectionIndex(7);
      mRawProbe.AddAccessibilityReadingInfo(UiAccessibility::ReadingInfo::STATE);
      mRawProbe.SetAccessibilityName("Hello 세계");
      mRawProbe.AddAccessibilityNameLanguageSpan(0u, 5u, "en-US");
      mRawProbe.ClearAccessibilityAttributes();
      status << "Clear: raw=" << (!HasRawAttribute(mRawProbe, "vendor-key") ? "PASS" : "FAIL")
             << ", initial=" << (mRawProbe.IsInitialAccessibilityHighlightRequested() ? "남음" : "제거")
             << ", collection=" << (mRawProbe.IsAccessibilityCollectionContainer() ? "남음" : "제거")
             << ", index=" << mRawProbe.GetAccessibilityCollectionIndex();
    }
    else
    {
      mRawProbe.SetAccessibilityName("raw attribute probe");
      mRawProbe.SetAccessibilityDescription("raw attribute API 복원 상태");
      status << "정형 metadata 복원 완료";
    }
    mRawStatus.SetText(status.str().c_str());
    mRawStatus.SetAccessibilityName(status.str().c_str());
    Log(status.str());
  }

  // 아래 자체검사는 wiki의 모든 View accessibility 데이터 API를 실제 타겟 프로세스에서 실행한다.
  // 외부 Screen Reader가 필요한 highlight/action 검사는 별도 페이지에서 수행하므로 여기서는 직접 호출하지 않는다.
  void RunAllApiSelfChecks()
  {
    View probe  = View::New();
    View target = View::New();

    // 1) 기본 문자열과 role setter/getter
    probe.SetAccessibilityName("이름");
    CheckApi("Set/GetAccessibilityName", probe.GetAccessibilityName() == "이름");
    probe.SetAccessibilityDescription("설명");
    CheckApi("Set/GetAccessibilityDescription", probe.GetAccessibilityDescription() == "설명");
    probe.SetAccessibilityValue("50%");
    CheckApi("Set/GetAccessibilityValue", probe.GetAccessibilityValue() == "50%");

    static const UiAccessibility::Role allRoles[] = {
      UiAccessibility::Role::ADJUSTABLE, UiAccessibility::Role::ALERT, UiAccessibility::Role::BUTTON,
      UiAccessibility::Role::CHECK_BOX, UiAccessibility::Role::COMBO_BOX, UiAccessibility::Role::CONTAINER,
      UiAccessibility::Role::DIALOG, UiAccessibility::Role::ENTRY, UiAccessibility::Role::HEADER,
      UiAccessibility::Role::IMAGE, UiAccessibility::Role::LINK, UiAccessibility::Role::LIST,
      UiAccessibility::Role::LIST_ITEM, UiAccessibility::Role::MENU, UiAccessibility::Role::MENU_BAR,
      UiAccessibility::Role::MENU_ITEM, UiAccessibility::Role::NONE, UiAccessibility::Role::NOTIFICATION,
      UiAccessibility::Role::PASSWORD_TEXT, UiAccessibility::Role::POPUP_MENU, UiAccessibility::Role::PROGRESS_BAR,
      UiAccessibility::Role::RADIO_BUTTON, UiAccessibility::Role::SCROLL_BAR, UiAccessibility::Role::SPIN_BUTTON,
      UiAccessibility::Role::TAB, UiAccessibility::Role::TAB_LIST, UiAccessibility::Role::TEXT,
      UiAccessibility::Role::TOGGLE_BUTTON, UiAccessibility::Role::TOOL_BAR, UiAccessibility::Role::SCENE_3D,
      UiAccessibility::Role::MODEL};
    bool rolesPassed = true;
    for(auto role : allRoles)
    {
      probe.SetAccessibilityRole(role);
      rolesPassed = rolesPassed && probe.GetAccessibilityRole() == role;
    }
    CheckApi("Set/GetAccessibilityRole 전체 enum", rolesPassed);

    // 2) 동작 속성과 대응 getter
    probe.SetAccessibilityRole(UiAccessibility::Role::BUTTON);
    probe.SetAccessibilityHidden(true);
    CheckApi("Set/IsAccessibilityHidden", probe.IsAccessibilityHidden());
    probe.SetAccessibilityHidden(false);
    probe.SetAccessibilityHighlightable(false);
    CheckApi("Set/IsAccessibilityHighlightable false", !probe.IsAccessibilityHighlightable());
    probe.ResetAccessibilityHighlightable();
    CheckApi("ResetAccessibilityHighlightable", probe.IsAccessibilityHighlightable());
    probe.SetAccessibilityScrollable(true);
    CheckApi("Set/IsAccessibilityScrollable", probe.IsAccessibilityScrollable());
    probe.SetAccessibilityModal(true);
    CheckApi("Set/IsAccessibilityModal", probe.IsAccessibilityModal());
    probe.SetAutomationId("wiki-api-probe");
    CheckApi("Set/GetAutomationId", probe.GetAutomationId() == "wiki-api-probe");

    // 3) ReadingInfo Add/Remove/Has/Clear와 모든 enum 값
    const UiAccessibility::ReadingInfo readingInfos[] = {
      UiAccessibility::ReadingInfo::NAME,
      UiAccessibility::ReadingInfo::ROLE,
      UiAccessibility::ReadingInfo::DESCRIPTION,
      UiAccessibility::ReadingInfo::STATE};
    probe.ClearAccessibilityReadingInfo();
    bool readingAdded = true;
    for(auto info : readingInfos)
    {
      probe.AddAccessibilityReadingInfo(info);
      readingAdded = readingAdded && probe.HasAccessibilityReadingInfo(info);
    }
    CheckApi("Add/HasAccessibilityReadingInfo 전체 enum", readingAdded);
    probe.RemoveAccessibilityReadingInfo(UiAccessibility::ReadingInfo::DESCRIPTION);
    CheckApi("RemoveAccessibilityReadingInfo", !probe.HasAccessibilityReadingInfo(UiAccessibility::ReadingInfo::DESCRIPTION));
    probe.ClearAccessibilityReadingInfo();
    bool readingCleared = true;
    for(auto info : readingInfos)
    {
      readingCleared = readingCleared && !probe.HasAccessibilityReadingInfo(info);
    }
    CheckApi("ClearAccessibilityReadingInfo", readingCleared);

    // 4) State Add/Remove/Has/Clear와 wiki에 설명된 모든 state
    const UiAccessibility::State states[] = {
      UiAccessibility::State::ENABLED,
      UiAccessibility::State::SELECTED,
      UiAccessibility::State::CHECKED,
      UiAccessibility::State::BUSY,
      UiAccessibility::State::EXPANDED};
    probe.ClearAccessibilityStates();
    bool statesAdded = true;
    for(auto state : states)
    {
      probe.AddAccessibilityState(state);
      statesAdded = statesAdded && probe.HasAccessibilityState(state);
    }
    CheckApi("Add/HasAccessibilityState 전체 enum", statesAdded);
    probe.RemoveAccessibilityState(UiAccessibility::State::BUSY);
    CheckApi("RemoveAccessibilityState", !probe.HasAccessibilityState(UiAccessibility::State::BUSY));
    probe.ClearAccessibilityStates();
    bool statesCleared = true;
    for(auto state : states)
    {
      statesCleared = statesCleared && !probe.HasAccessibilityState(state);
    }
    CheckApi("ClearAccessibilityStates", statesCleared);

    // 5) relation 전체 유효 enum을 같은 target에 추가하고 Has/Remove/Clear를 검증한다.
    const UiAccessibility::RelationType relations[] = {
      UiAccessibility::RelationType::LABEL_FOR, UiAccessibility::RelationType::LABELLED_BY,
      UiAccessibility::RelationType::CONTROLLER_FOR, UiAccessibility::RelationType::CONTROLLED_BY,
      UiAccessibility::RelationType::MEMBER_OF, UiAccessibility::RelationType::TOOLTIP_FOR,
      UiAccessibility::RelationType::NODE_CHILD_OF, UiAccessibility::RelationType::NODE_PARENT_OF,
      UiAccessibility::RelationType::EXTENDED, UiAccessibility::RelationType::FLOWS_TO,
      UiAccessibility::RelationType::FLOWS_FROM, UiAccessibility::RelationType::SUBWINDOW_OF,
      UiAccessibility::RelationType::EMBEDS, UiAccessibility::RelationType::EMBEDDED_BY,
      UiAccessibility::RelationType::POPUP_FOR, UiAccessibility::RelationType::PARENT_WINDOW_OF,
      UiAccessibility::RelationType::DESCRIPTION_FOR, UiAccessibility::RelationType::DESCRIBED_BY,
      UiAccessibility::RelationType::DETAILS, UiAccessibility::RelationType::DETAILS_FOR,
      UiAccessibility::RelationType::ERROR_MESSAGE, UiAccessibility::RelationType::ERROR_FOR};
    bool relationsAdded = true;
    for(auto relation : relations)
    {
      probe.AddAccessibilityRelation(relation, target);
      relationsAdded = relationsAdded && probe.HasAccessibilityRelation(relation, target);
    }
    CheckApi("Add/HasAccessibilityRelation 전체 enum", relationsAdded);
    probe.RemoveAccessibilityRelation(UiAccessibility::RelationType::LABEL_FOR, target);
    CheckApi("RemoveAccessibilityRelation", !probe.HasAccessibilityRelation(UiAccessibility::RelationType::LABEL_FOR, target));
    probe.ClearAccessibilityRelations();
    CheckApi("ClearAccessibilityRelations", !probe.HasAccessibilityRelation(UiAccessibility::RelationType::ERROR_FOR, target));

    // 6) 번역 binding의 default domain/domain overload, resource id getter와 Clear API
    probe.SetTranslatableAccessibilityName("IDS_WIKI_WIFI_NAME");
    probe.SetTranslatableAccessibilityDescription("IDS_WIKI_WIFI_DESCRIPTION");
    UiLocalizationManager::Get().RefreshBindings();
    CheckApi("SetTranslatableAccessibilityName default domain",
             probe.GetTranslatableAccessibilityName() == "IDS_WIKI_WIFI_NAME" &&
               probe.GetAccessibilityName() == "기본 도메인 와이파이" &&
               probe.GetTranslatableAccessibilityDescription() == "IDS_WIKI_WIFI_DESCRIPTION");
    probe.SetTranslatableAccessibilityName("IDS_WIKI_WIFI_NAME", "wiki-sample");
    probe.SetTranslatableAccessibilityDescription("IDS_WIKI_WIFI_DESCRIPTION", "wiki-sample");
    UiLocalizationManager::Get().RefreshBindings();
    CheckApi("Set/GetTranslatableAccessibility name/description domain",
             probe.GetAccessibilityName() == "와이파이 설정" &&
               probe.GetAccessibilityDescription() == "사용 가능한 네트워크를 엽니다" &&
               probe.GetTranslatableAccessibilityDescription() == "IDS_WIKI_WIFI_DESCRIPTION");
    probe.ClearTranslatableAccessibilityName();
    probe.ClearTranslatableAccessibilityDescription();
    CheckApi("ClearTranslatableAccessibility name/description",
             probe.GetTranslatableAccessibilityName().Empty() && probe.GetTranslatableAccessibilityDescription().Empty());

    // 7) language span은 UTF-8 byte가 아니라 Unicode code point 범위다.
    probe.SetAccessibilityName("Hello 세계");
    bool validNameSpans = probe.AddAccessibilityNameLanguageSpan(0u, 5u, "en-US") &&
                          probe.AddAccessibilityNameLanguageSpan(6u, 2u, "ko-KR");
    bool rejectedNameSpans = !probe.AddAccessibilityNameLanguageSpan(0u, 0u, "en-US") &&
                             !probe.AddAccessibilityNameLanguageSpan(0u, 1u, "") &&
                             !probe.AddAccessibilityNameLanguageSpan(20u, 1u, "ko-KR") &&
                             !probe.AddAccessibilityNameLanguageSpan(1u, 2u, "en-US");
    CheckApi("AddAccessibilityNameLanguageSpan valid/invalid", validNameSpans && rejectedNameSpans);
    probe.ClearAccessibilityNameLanguageSpans();
    CheckApi("ClearAccessibilityNameLanguageSpans",
             probe.AddAccessibilityNameLanguageSpan(0u, 5u, "en-US"));

    probe.SetAccessibilityDescription("English 한국어");
    bool validDescriptionSpans = probe.AddAccessibilityDescriptionLanguageSpan(0u, 7u, "en-US") &&
                                 probe.AddAccessibilityDescriptionLanguageSpan(8u, 3u, "ko-KR");
    CheckApi("AddAccessibilityDescriptionLanguageSpan", validDescriptionSpans);
    probe.ClearAccessibilityDescriptionLanguageSpans();
    CheckApi("ClearAccessibilityDescriptionLanguageSpans",
             probe.AddAccessibilityDescriptionLanguageSpan(0u, 7u, "en-US"));

    // 명시적 name/description을 다시 설정하면 translation binding과 기존 span이 해제되는 규칙도 확인한다.
    probe.SetTranslatableAccessibilityName("IDS_WIKI_WIFI_NAME", "wiki-sample");
    probe.SetAccessibilityName("World");
    bool nameResetRule = probe.GetTranslatableAccessibilityName().Empty() &&
                         probe.AddAccessibilityNameLanguageSpan(0u, 5u, "en-US");
    CheckApi("명시적 name 설정의 binding/span clear 규칙", nameResetRule);

    // 8) collection container/index getter와 Clear, Set(-1) 동작
    probe.SetAccessibilityCollectionContainer(true);
    CheckApi("Set/IsAccessibilityCollectionContainer", probe.IsAccessibilityCollectionContainer());
    probe.SetAccessibilityCollectionContainer(false);
    CheckApi("SetAccessibilityCollectionContainer false", !probe.IsAccessibilityCollectionContainer());
    probe.SetAccessibilityCollectionContainer(true);
    probe.SetAccessibilityCollectionIndex(4);
    CheckApi("Set/GetAccessibilityCollectionIndex", probe.GetAccessibilityCollectionIndex() == 4);
    probe.ClearAccessibilityCollectionIndex();
    CheckApi("ClearAccessibilityCollectionIndex", probe.GetAccessibilityCollectionIndex() == -1);
    probe.SetAccessibilityCollectionIndex(3);
    probe.SetAccessibilityCollectionIndex(-1);
    CheckApi("SetAccessibilityCollectionIndex(-1)", probe.GetAccessibilityCollectionIndex() == -1);

    // 9) initial highlight metadata setter/getter. 실제 선택 여부는 앱 시작과 6번 화면에서 외부 검증한다.
    probe.SetRequestInitialAccessibilityHighlight(true);
    CheckApi("Set/IsInitialAccessibilityHighlightRequested true", probe.IsInitialAccessibilityHighlightRequested());
    probe.SetRequestInitialAccessibilityHighlight(false);
    CheckApi("SetRequestInitialAccessibilityHighlight false", !probe.IsInitialAccessibilityHighlightRequested());

    // 10) raw attribute export와 typed attribute까지 포함하는 Clear 범위
    probe.AppendAccessibilityAttribute("vendor-key", "vendor-value");
    CheckApi("AppendAccessibilityAttribute export", HasRawAttribute(probe, "vendor-key", "vendor-value"));
    probe.RemoveAccessibilityAttribute("vendor-key");
    CheckApi("RemoveAccessibilityAttribute export", !HasRawAttribute(probe, "vendor-key"));
    probe.AppendAccessibilityAttribute("vendor-key", "vendor-value");
    probe.SetRequestInitialAccessibilityHighlight(true);
    probe.SetAccessibilityCollectionContainer(true);
    probe.SetAccessibilityCollectionIndex(9);
    probe.ClearAccessibilityAttributes();
    CheckApi("ClearAccessibilityAttributes raw/typed",
             !HasRawAttribute(probe, "vendor-key") &&
               !probe.IsInitialAccessibilityHighlightRequested() &&
               !probe.IsAccessibilityCollectionContainer() &&
               probe.GetAccessibilityCollectionIndex() == -1);
  }

  void CheckApi(const char* api, bool passed)
  {
    ++mSelfCheckTotal;
    if(passed)
    {
      ++mSelfCheckPassed;
    }
    else
    {
      mSelfCheckFailures.emplace_back(api);
    }
    Log(std::string(passed ? "SELF PASS " : "SELF FAIL ") + api);
  }

  void UpdateSelfCheckReport()
  {
    std::ostringstream report;
    report << "wiki API 타겟 자체검사\n\n"
           << "결과: " << mSelfCheckPassed << '/' << mSelfCheckTotal
           << (mSelfCheckPassed == mSelfCheckTotal ? " PASS" : " FAIL") << "\n\n"
           << "검사 범위:\n"
           << "- 기본 name/description/value/role와 getter\n"
           << "- hidden/highlightable/scrollable/modal/automation id\n"
           << "- ReadingInfo 4종, State 5종\n"
           << "- RelationType 전체 유효값\n"
           << "- 번역 binding과 language span\n"
           << "- collection, initial highlight metadata\n"
           << "- raw attribute append/remove/clear\n";
    if(!mSelfCheckFailures.empty())
    {
      report << "\n실패 API:";
      for(const auto& failure : mSelfCheckFailures)
      {
        report << "\n- " << failure;
      }
    }
    mSelfCheckReport.SetText(report.str().c_str());
    mSelfCheckReport.SetAccessibilityDescription(report.str().c_str());
  }

  void UpdateHighlightStatus()
  {
    if(!mHighlightStatus)
    {
      return;
    }
    const int readingCount = static_cast<int>(std::count(mReadingStatusSeen.begin(), mReadingStatusSeen.end(), true));
    std::ostringstream text;
    text << "Grab=" << (mGrabHighlightSucceeded ? "true" : "false")
         << " Clear=" << (mClearHighlightSucceeded ? "true" : "false")
         << "\nHighlightedSignal true/false=" << mHighlightTrueCount << '/' << mHighlightFalseCount
         << "\nReadingStatus=" << readingCount << "/5";
    mHighlightStatus.SetText(text.str().c_str());
    mHighlightStatus.SetAccessibilityName(text.str().c_str());
  }

  void UpdateVirtualStatus()
  {
    if(!mVirtualStatus)
    {
      return;
    }
    std::ostringstream text;
    text << "동적 query N/D/V=" << mVirtualNameQueryCount << '/' << mVirtualDescriptionQueryCount << '/' << mVirtualValueQueryCount
         << "\naction activate/value/escape=" << mVirtualActivateCount << '/' << mVirtualValueCount << '/' << mVirtualEscapeCount
         << "\nscroll/pan/zoom=" << mVirtualScrollCount << '/' << mVirtualPanCount << '/' << mVirtualZoomCount;
    mVirtualStatus.SetText(text.str().c_str());
    mVirtualStatus.SetAccessibilityName(text.str().c_str());
  }

  void OnBridgeChanged()
  {
    RefreshOverall("bridge or screen reader signal");
  }

  void RefreshOverall(const char* reason = nullptr)
  {
    if(!mOverall || !mBridgeStatus)
    {
      return;
    }

    const bool bridgeUp     = AdaptorAccessibility::IsEnabled();
    const bool screenReader = AdaptorAccessibility::IsScreenReaderEnabled();
    std::ostringstream bridge;
    bridge << "AT-SPI bridge=" << (bridgeUp ? "UP" : "DOWN")
           << " / Screen Reader=" << (screenReader ? "ON" : "OFF");
    mBridgeStatus.SetText(bridge.str().c_str());
    mBridgeStatus.SetAccessibilityName(bridge.str().c_str());

    const bool allReadingStatuses = std::all_of(mReadingStatusSeen.begin(), mReadingStatusSeen.end(), [](bool seen) { return seen; });
    const bool externalPassed = mHighlightTrueCount > 0 && mHighlightFalseCount > 0 &&
                                mGrabHighlightSucceeded && mClearHighlightSucceeded && allReadingStatuses &&
                                mVirtualNameQueryCount > 0 && mVirtualDescriptionQueryCount > 0 && mVirtualValueQueryCount > 0 &&
                                mVirtualActivateCount > 0 && mVirtualValueCount > 0 && mVirtualEscapeCount > 0 &&
                                mVirtualScrollCount > 0 && mVirtualPanCount > 0 && mVirtualZoomCount > 0;
    const bool selfPassed = mSelfCheckPassed == mSelfCheckTotal && mSelfCheckTotal > 0;
    const bool passed = selfPassed && bridgeUp && screenReader && externalPassed && !mHiddenSentinelExposed;
    const bool failed = !selfPassed || mHiddenSentinelExposed;

    const char* result = failed ? "FAIL" : (passed ? "PASS" : "WAITING");
    std::ostringstream summary;
    summary << "SELF " << mSelfCheckPassed << '/' << mSelfCheckTotal
            << " | TARGET " << (externalPassed ? "DONE" : "TODO")
            << " | HIDDEN " << (mHiddenSentinelExposed ? "FAIL" : "OK")
            << " | RESULT " << result;
    mOverall.SetText(summary.str().c_str());
    mOverall.SetAccessibilityName(summary.str().c_str());
    mOverall.SetTextColor(UiColor(failed ? 0xB03A2Eu : (passed ? 0x196F3Du : 0xA04000u)));

    if(reason)
    {
      Log(std::string(reason) + "; " + bridge.str() + "; result=" + result);
    }
    if(passed && !mPassLogged)
    {
      Log("RESULT=PASS wiki APIs and real target accessibility path verified");
      mPassLogged = true;
    }
  }

  void OnKeyEvent(Window, const KeyEvent& event)
  {
    if(event.GetState() == KeyEvent::DOWN &&
       (IsKey(event, DALI_KEY_ESCAPE) || IsKey(event, DALI_KEY_BACK)))
    {
      mApplication.Quit();
    }
  }

private:
  Application& mApplication;

  View                    mPageHost;
  std::vector<StackLayout> mPages;
  int                     mCurrentPage{0};
  Label                   mTitle;
  Label                   mBridgeStatus;
  Label                   mPageGuide;
  Label                   mSelfCheckReport;
  Label                   mOverall;
  View                    mNextPage;
  Label                   mNextPageLabel;

  View  mSemanticTarget;
  Label mSemanticLabel;
  View  mBehaviorTarget;
  Label mBehaviorLabel;
  int   mRoleIndex{-1};
  int   mBehaviorStep{0};

  View  mStateTarget;
  Label mStateLabel;
  View  mReadingTarget;
  Label mReadingLabel;
  int   mStateStep{0};
  int   mReadingStep{0};

  View  mRelationTitle;
  View  mRelationInput;
  View  mRelationError;
  View  mTranslationTarget;
  Label mTranslationLabel;
  View  mLanguageTarget;
  Label mLanguageLabel;
  int   mRelationStep{0};
  bool  mTranslationEnabled{true};

  StackLayout       mCollection;
  std::vector<View> mCollectionItems;
  Label             mCollectionStatus;
  int               mCollectionStep{0};

  View                mHighlightTarget;
  Label               mHighlightStatus;
  int                 mHighlightTrueCount{0};
  int                 mHighlightFalseCount{0};
  bool                mGrabHighlightSucceeded{false};
  bool                mClearHighlightSucceeded{false};
  std::array<bool, 5> mReadingStatusSeen{};

  View                       mVirtualTarget;
  WikiAccessibilityViewImpl* mVirtualImpl{nullptr};
  Label                      mVirtualLabel;
  View                       mScrollContainer;
  WikiAccessibilityViewImpl* mScrollContainerImpl{nullptr};
  View                       mScrollChild;
  Label                      mVirtualStatus;
  int                        mVirtualValue{5};
  int                        mVirtualNameQueryCount{0};
  int                        mVirtualDescriptionQueryCount{0};
  int                        mVirtualValueQueryCount{0};
  int                        mVirtualActivateCount{0};
  int                        mVirtualValueCount{0};
  int                        mVirtualEscapeCount{0};
  int                        mVirtualScrollCount{0};
  int                        mVirtualPanCount{0};
  int                        mVirtualZoomCount{0};

  View  mRawProbe;
  Label mRawStatus;
  int   mRawStep{0};

  View                     mHiddenSentinel;
  bool                     mHiddenSentinelExposed{false};
  int                      mSelfCheckTotal{0};
  int                      mSelfCheckPassed{0};
  std::vector<std::string> mSelfCheckFailures;
  bool                     mPassLogged{false};
};
} // namespace

int DALI_EXPORT_API main(int argc, char** argv)
{
  Application                 application = Application::New(&argc, &argv);
  AccessibilityWikiTargetTest test(application);
  application.MainLoop();
  return 0;
}
