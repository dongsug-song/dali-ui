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

// This sample intentionally does not call Accessible::Get() or View::DoAction().
// Its counters change only when a real accessibility client highlights a View or
// invokes one of the actions exported through the adaptor accessibility bridge.

#include <dali-ui-foundation/dali-ui-foundation.h>
#include <dali-ui-foundation/public-api/layouts/stack-layout.h>
#include <dali-ui-foundation/public-api/views/text-controls/label.h>
#include <dali-ui-foundation/public-api/views/view-impl.h>
#include <dali/devel-api/object/type-registry-helper.h>
#include <dali/integration-api/adaptor-framework/accessibility/accessibility-bridge.h>
#include <dali/integration-api/adaptor-framework/accessibility/accessibility-service.h>
#include <dali/integration-api/debug.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>

namespace
{
using namespace Dali;
using namespace Dali::Ui;

namespace AdaptorAccessibility = Dali::Integration::Accessibility;
namespace UiAccessibility      = Dali::Ui::Accessibility;

constexpr float CONTROL_HEIGHT = 72.0f;

enum class ControlKind : std::size_t
{
  ACTIVATE = 0u,
  ADJUSTABLE,
  TOGGLE,
  HIDDEN_SENTINEL,
  COUNT
};

constexpr std::size_t VISIBLE_CONTROL_COUNT = static_cast<std::size_t>(ControlKind::HIDDEN_SENTINEL);

enum class PropertyKind : std::size_t
{
  NAME = 0u,
  DESCRIPTION,
  VALUE,
  COUNT
};

const char* ToString(ControlKind kind)
{
  switch(kind)
  {
    case ControlKind::ACTIVATE:
      return "activate button";
    case ControlKind::ADJUSTABLE:
      return "adjustable value";
    case ControlKind::TOGGLE:
      return "toggle checkbox";
    case ControlKind::HIDDEN_SENTINEL:
      return "accessibility-hidden sentinel";
    default:
      return "unknown control";
  }
}

const char* ToString(UiAccessibility::ReadingStatus status)
{
  switch(status)
  {
    case UiAccessibility::ReadingStatus::SKIPPED:
      return "skipped";
    case UiAccessibility::ReadingStatus::PAUSED:
      return "paused";
    case UiAccessibility::ReadingStatus::RESUMED:
      return "resumed";
    case UiAccessibility::ReadingStatus::CANCELLED:
      return "cancelled";
    case UiAccessibility::ReadingStatus::STOPPED:
      return "stopped";
  }
  return "unknown";
}

const char* ToString(PropertyKind property)
{
  switch(property)
  {
    case PropertyKind::NAME:
      return "name";
    case PropertyKind::DESCRIPTION:
      return "description";
    case PropertyKind::VALUE:
      return "value";
    default:
      return "unknown property";
  }
}

void Log(const std::string& message)
{
  DALI_LOG_RELEASE_INFO("[A11Y_TARGET] %s\n", message.c_str());
  std::cout << "[A11Y_TARGET] " << message << std::endl;
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

class TargetControlImpl : public ViewImpl
{
public:
  using Ptr              = Dali::IntrusivePtr<TargetControlImpl>;
  using ActionCallback   = std::function<bool(ControlKind, bool)>;
  using PropertyCallback = std::function<bool(ControlKind, PropertyKind, Dali::String&)>;

  static Ptr New(ControlKind kind = ControlKind::ACTIVATE,
                 ActionCallback actionCallback = {},
                 PropertyCallback propertyCallback = {})
  {
    return Ptr(new TargetControlImpl(kind, std::move(actionCallback), std::move(propertyCallback)));
  }

  bool OnAccessibilityActivate() override
  {
    if(mKind == ControlKind::ADJUSTABLE || !mActionCallback)
    {
      return false;
    }
    return mActionCallback(mKind, true);
  }

  bool OnAccessibilityValueChange(bool isIncreased) override
  {
    if(mKind != ControlKind::ADJUSTABLE || !mActionCallback)
    {
      return false;
    }
    return mActionCallback(mKind, isIncreased);
  }

  bool OnAccessibilityRequestName(Dali::String& value) override
  {
    return RequestProperty(PropertyKind::NAME, value);
  }

  bool OnAccessibilityRequestDescription(Dali::String& value) override
  {
    return RequestProperty(PropertyKind::DESCRIPTION, value);
  }

  bool OnAccessibilityRequestValue(Dali::String& value) override
  {
    return RequestProperty(PropertyKind::VALUE, value);
  }

protected:
  ~TargetControlImpl() override = default;

private:
  TargetControlImpl(ControlKind kind, ActionCallback actionCallback, PropertyCallback propertyCallback)
  : mKind(kind),
    mActionCallback(std::move(actionCallback)),
    mPropertyCallback(std::move(propertyCallback))
  {
  }

  bool RequestProperty(PropertyKind property, Dali::String& value)
  {
    return mPropertyCallback && mPropertyCallback(mKind, property, value);
  }

  ControlKind      mKind;
  ActionCallback   mActionCallback;
  PropertyCallback mPropertyCallback;
};

View CreateTargetControl(ControlKind kind = ControlKind::ACTIVATE,
                         TargetControlImpl::ActionCallback actionCallback = {},
                         TargetControlImpl::PropertyCallback propertyCallback = {})
{
  auto impl = TargetControlImpl::New(kind, std::move(actionCallback), std::move(propertyCallback));
  View view(*impl);
  impl->Initialize();
  return view;
}

Dali::BaseHandle CreateRegisteredTargetControl()
{
  return CreateTargetControl();
}

DALI_TYPE_REGISTRATION_BEGIN(TargetControlImpl, Dali::Ui::ViewImpl, CreateRegisteredTargetControl)
DALI_TYPE_REGISTRATION_END()

class AccessibilityTargetTest : public ConnectionTracker
{
public:
  explicit AccessibilityTargetTest(Application& application)
  : mApplication(application)
  {
    mApplication.InitSignal().Connect(this, &AccessibilityTargetTest::OnInit);
  }

private:
  void OnInit(Application application)
  {
    Window window = application.GetWindow();
    window.SetBackgroundColor(Color::WHITE);
    window.KeyEventSignal().Connect(this, &AccessibilityTargetTest::OnKeyEvent);

    StackLayout root = StackLayout::New(StackOrientation::VERTICAL);
    root.SetRequestedWidth(MATCH_PARENT);
    root.SetRequestedHeight(MATCH_PARENT);
    root.SetSpacing(8.0f);
    root.SetPadding(Extents(20, 20, 16, 16));
    root.SetAccessibilityRole(UiAccessibility::Role::CONTAINER);
    root.SetAccessibilityHighlightable(false);
    root.SetAccessibilityCollectionContainer(true);

    Label title = MakeLabel("DALi accessibility target test", 24.0f, 0x17202Au, 36.0f);
    title.SetAccessibilityRole(UiAccessibility::Role::HEADER);
    title.SetAccessibilityName("DALi accessibility target test");
    root.Add(title);

    Label instructions = MakeLabel(
      "Turn Screen Reader on. Swipe through all three test controls, then activate the button, "
      "adjust the value, and toggle the checkbox.",
      14.0f,
      0x34495Eu,
      76.0f,
      true);
    instructions.SetAccessibilityRole(UiAccessibility::Role::TEXT);
    instructions.SetAccessibilityName("Test instructions");
    instructions.SetAccessibilityDescription(
      "Swipe through three controls. Activate the button, adjust the value, and toggle the checkbox.");
    root.Add(instructions);

    mBridgeStatus = MakeLabel("AT-SPI bridge: checking", 15.0f, 0x17202Au, 48.0f, true);
    mBridgeStatus.SetAccessibilityRole(UiAccessibility::Role::NOTIFICATION);
    root.Add(mBridgeStatus);

    mLastEvent = MakeLabel("Last real-client event: none", 14.0f, 0x5D6D7Eu, 44.0f, true);
    mLastEvent.SetAccessibilityRole(UiAccessibility::Role::NOTIFICATION);
    root.Add(mLastEvent);

    mActivate = AddControl(root,
                           ControlKind::ACTIVATE,
                           UiAccessibility::Role::BUTTON,
                           "Activation test button",
                           "Double tap. The application callback counter must increase.",
                           "ACTIVATE: waiting for Screen Reader double tap",
                           0xD6EAF8u,
                           mActivateLabel);
    mActivate.SetAccessibilityValue("0 activations");
    mActivate.SetRequestInitialAccessibilityHighlight(true);
    mActivate.SetAccessibilityCollectionIndex(0);
    mActivate.AddAccessibilityRelation(UiAccessibility::RelationType::DESCRIBED_BY, instructions);

    mAdjustable = AddControl(root,
                             ControlKind::ADJUSTABLE,
                             UiAccessibility::Role::ADJUSTABLE,
                             "Adjustable test value",
                             "Use the Screen Reader increase or decrease gesture.",
                             "ADJUSTABLE: value 5 of 10",
                             0xFCF3CFu,
                             mAdjustableLabel);
    mAdjustable.SetAccessibilityValue("5 of 10");
    mAdjustable.SetAccessibilityCollectionIndex(1);
    mAdjustable.AddAccessibilityRelation(UiAccessibility::RelationType::DESCRIBED_BY, instructions);

    mToggle = AddControl(root,
                         ControlKind::TOGGLE,
                         UiAccessibility::Role::CHECK_BOX,
                         "Toggle test checkbox",
                         "Double tap. The checked accessibility state must change.",
                         "CHECKBOX: unchecked",
                         0xD5F5E3u,
                         mToggleLabel);
    mToggle.SetAccessibilityValue("unchecked");
    mToggle.SetAccessibilityCollectionIndex(2);
    mToggle.AddAccessibilityRelation(UiAccessibility::RelationType::DESCRIBED_BY, instructions);

    // A real client must never expose this object during navigation. Keeping it
    // on scene exercises the bridge's accessibility-hidden filtering rather
    // than ordinary Actor visibility or scene disconnection.
    View hiddenSentinel = CreateTargetControl(
      ControlKind::HIDDEN_SENTINEL,
      {},
      [this](ControlKind callbackKind, PropertyKind property, Dali::String& value) {
        return OnAccessibilityPropertyRequested(callbackKind, property, value);
      });
    hiddenSentinel.SetRequestedWidth(MATCH_PARENT);
    hiddenSentinel.SetRequestedHeight(1.0f);
    hiddenSentinel.SetAccessibilityRole(UiAccessibility::Role::NOTIFICATION);
    hiddenSentinel.SetAccessibilityName("FAIL: accessibility-hidden sentinel was exposed");
    hiddenSentinel.SetAccessibilityDescription("This object must not be announced by the Screen Reader.");
    hiddenSentinel.SetAccessibilityHidden(true);
    hiddenSentinel.SetAutomationId("accessibility-hidden-sentinel");
    root.Add(hiddenSentinel);

    mOverall = MakeLabel("RESULT: WAITING FOR REAL SCREEN READER EVENTS", 16.0f, 0xA04000u, 54.0f, true);
    mOverall.SetAccessibilityRole(UiAccessibility::Role::NOTIFICATION);
    root.Add(mOverall);

    window.Add(root);

    auto& bridge = AdaptorAccessibility::Bridge::EnabledSignal();
    bridge.Connect(this, &AccessibilityTargetTest::OnBridgeStatusChanged);
    AdaptorAccessibility::Bridge::DisabledSignal().Connect(this, &AccessibilityTargetTest::OnBridgeStatusChanged);
    AdaptorAccessibility::Bridge::ScreenReaderEnabledSignal().Connect(this, &AccessibilityTargetTest::OnScreenReaderStatusChanged);
    AdaptorAccessibility::Bridge::ScreenReaderDisabledSignal().Connect(this, &AccessibilityTargetTest::OnScreenReaderStatusChanged);

    RefreshStatus("application initialized");
  }

  View AddControl(StackLayout& root,
                  ControlKind kind,
                  UiAccessibility::Role role,
                  const char* name,
                  const char* description,
                  const char* visibleText,
                  uint32_t backgroundColor,
                  Label& visibleLabel)
  {
    View control = CreateTargetControl(
      kind,
      [this](ControlKind callbackKind, bool direction) {
        return OnAccessibilityAction(callbackKind, direction);
      },
      [this](ControlKind callbackKind, PropertyKind property, Dali::String& value) {
        return OnAccessibilityPropertyRequested(callbackKind, property, value);
      });
    control.SetRequestedWidth(MATCH_PARENT);
    control.SetRequestedHeight(CONTROL_HEIGHT);
    control.SetBackgroundColor(UiColor(backgroundColor));
    control.SetFocusable(true);
    control.SetAccessibilityRole(role);
    control.SetAccessibilityName(name);
    control.SetAccessibilityDescription(description);
    control.SetAutomationId(ToString(kind));
    control.AddAccessibilityState(UiAccessibility::State::ENABLED);
    control.AddAccessibilityReadingInfo(UiAccessibility::ReadingInfo::NAME);
    control.AddAccessibilityReadingInfo(UiAccessibility::ReadingInfo::ROLE);
    control.AddAccessibilityReadingInfo(UiAccessibility::ReadingInfo::DESCRIPTION);
    control.AddAccessibilityReadingInfo(UiAccessibility::ReadingInfo::STATE);

    visibleLabel = MakeLabel(visibleText, 16.0f, 0x17202Au, CONTROL_HEIGHT, true);
    visibleLabel.SetRequestedX(14.0f);
    visibleLabel.SetAccessibilityHidden(true);
    control.Add(visibleLabel);

    ObserveRealClientEvents(control, kind);
    root.Add(control);
    return control;
  }

  void ObserveRealClientEvents(View control, ControlKind kind)
  {
    control.AccessibilityHighlightedSignal().Connect(this, [this, kind](View, bool highlighted) {
      if(!highlighted)
      {
        return;
      }

      ++mHighlightCount[static_cast<std::size_t>(kind)];
      std::ostringstream message;
      message << "highlight received for " << ToString(kind);
      RecordRealClientEvent(message.str());
    });

    control.AccessibilityReadingStatusChangedSignal().Connect(
      this,
      [this, kind](View, UiAccessibility::ReadingStatus status) {
        std::ostringstream message;
        message << "reading " << ToString(status) << " for " << ToString(kind);
        RecordRealClientEvent(message.str());
      });
  }

  bool OnAccessibilityAction(ControlKind kind, bool direction)
  {
    std::ostringstream event;
    switch(kind)
    {
      case ControlKind::ACTIVATE:
      {
        ++mActivateCount;
        event << "activate callback count=" << mActivateCount;

        std::ostringstream text;
        text << "ACTIVATE: callback received " << mActivateCount << " time" << (mActivateCount == 1 ? "" : "s");
        mActivateLabel.SetText(text.str().c_str());

        std::ostringstream value;
        value << mActivateCount << " activation" << (mActivateCount == 1 ? "" : "s");
        mActivate.SetAccessibilityValue(value.str().c_str());

        AdaptorAccessibility::Say(
          "DALi accessibility activation callback received",
          true,
          [](std::string status) {
            Log(std::string("Say callback: ") + status);
          });
        break;
      }
      case ControlKind::ADJUSTABLE:
      {
        ++mAdjustCount;
        mAdjustValue = std::clamp(mAdjustValue + (direction ? 1 : -1), 0, 10);
        event << (direction ? "increment" : "decrement") << " callback value=" << mAdjustValue;

        std::ostringstream text;
        text << "ADJUSTABLE: value " << mAdjustValue << " of 10 (callbacks " << mAdjustCount << ')';
        mAdjustableLabel.SetText(text.str().c_str());

        std::ostringstream value;
        value << mAdjustValue << " of 10";
        mAdjustable.SetAccessibilityValue(value.str().c_str());
        break;
      }
      case ControlKind::TOGGLE:
      {
        ++mToggleCount;
        mChecked = !mChecked;
        event << "toggle callback checked=" << (mChecked ? "true" : "false");

        if(mChecked)
        {
          mToggle.AddAccessibilityState(UiAccessibility::State::CHECKED);
        }
        else
        {
          mToggle.RemoveAccessibilityState(UiAccessibility::State::CHECKED);
        }
        mToggleLabel.SetText(mChecked ? "CHECKBOX: checked" : "CHECKBOX: unchecked");
        mToggle.SetAccessibilityValue(mChecked ? "checked" : "unchecked");
        break;
      }
      default:
        return false;
    }

    RecordRealClientEvent(event.str());
    return true;
  }

  bool OnAccessibilityPropertyRequested(ControlKind kind, PropertyKind property, Dali::String& value)
  {
    const std::size_t controlIndex  = static_cast<std::size_t>(kind);
    const std::size_t propertyIndex = static_cast<std::size_t>(property);
    int&              queryCount    = mPropertyQueryCount[controlIndex][propertyIndex];
    ++queryCount;

    switch(property)
    {
      case PropertyKind::NAME:
        switch(kind)
        {
          case ControlKind::ACTIVATE:
            value = "Activation test button";
            break;
          case ControlKind::ADJUSTABLE:
            value = "Adjustable test value";
            break;
          case ControlKind::TOGGLE:
            value = "Toggle test checkbox";
            break;
          case ControlKind::HIDDEN_SENTINEL:
            value = "FAIL: accessibility-hidden sentinel was exposed";
            break;
          default:
            return false;
        }
        break;
      case PropertyKind::DESCRIPTION:
        switch(kind)
        {
          case ControlKind::ACTIVATE:
            value = "Double tap. The application callback counter must increase.";
            break;
          case ControlKind::ADJUSTABLE:
            value = "Use the Screen Reader increase or decrease gesture.";
            break;
          case ControlKind::TOGGLE:
            value = "Double tap. The checked accessibility state must change.";
            break;
          case ControlKind::HIDDEN_SENTINEL:
            value = "This object must not be announced by the Screen Reader.";
            break;
          default:
            return false;
        }
        break;
      case PropertyKind::VALUE:
      {
        std::ostringstream dynamicValue;
        switch(kind)
        {
          case ControlKind::ACTIVATE:
            dynamicValue << mActivateCount << " activation" << (mActivateCount == 1 ? "" : "s");
            break;
          case ControlKind::ADJUSTABLE:
            dynamicValue << mAdjustValue << " of 10";
            break;
          case ControlKind::TOGGLE:
            dynamicValue << (mChecked ? "checked" : "unchecked");
            break;
          case ControlKind::HIDDEN_SENTINEL:
            dynamicValue << "hidden";
            break;
          default:
            return false;
        }
        value = dynamicValue.str().c_str();
        break;
      }
      default:
        return false;
    }

    if(kind == ControlKind::HIDDEN_SENTINEL)
    {
      mHiddenSentinelExposed = true;
      if(queryCount == 1)
      {
        Log(std::string("RESULT=FAIL: external client queried hidden sentinel ") + ToString(property));
        if(mOverall)
        {
          mOverall.SetText("RESULT: FAIL | accessibility-hidden sentinel was queried");
          mOverall.SetAccessibilityName("Result fail. Accessibility-hidden sentinel was queried.");
          mOverall.SetTextColor(UiColor(0xB03A2Eu));
        }
      }
    }
    else if(queryCount == 1)
    {
      std::ostringstream message;
      message << "external property query: " << ToString(kind) << ' ' << ToString(property);
      Log(message.str());
    }
    return true;
  }

  void OnBridgeStatusChanged()
  {
    RefreshStatus("AT-SPI bridge state signal received");
  }

  void OnScreenReaderStatusChanged()
  {
    RefreshStatus("Screen Reader state signal received");
  }

  void RecordRealClientEvent(const std::string& event)
  {
    mLastEvent.SetText((std::string("Last real-client event: ") + event).c_str());
    mLastEvent.SetAccessibilityName(event.c_str());
    Log(event);
    RefreshStatus();
  }

  void RefreshStatus(const char* reason = nullptr)
  {
    const bool bridgeUp           = AdaptorAccessibility::IsEnabled();
    const bool screenReaderActive = AdaptorAccessibility::IsScreenReaderEnabled();

    std::ostringstream bridgeText;
    bridgeText << "AT-SPI bridge: " << (bridgeUp ? "UP" : "DOWN")
               << "   Screen Reader: " << (screenReaderActive ? "ON" : "OFF");
    mBridgeStatus.SetText(bridgeText.str().c_str());
    mBridgeStatus.SetAccessibilityName(bridgeText.str().c_str());

    const bool allHighlighted = std::all_of(mHighlightCount.begin(),
                                            mHighlightCount.begin() + VISIBLE_CONTROL_COUNT,
                                            [](int count) { return count > 0; });
    const bool allNamesQueried = std::all_of(mPropertyQueryCount.begin(),
                                             mPropertyQueryCount.begin() + VISIBLE_CONTROL_COUNT,
                                             [](const auto& counts) {
                                               return counts[static_cast<std::size_t>(PropertyKind::NAME)] > 0;
                                             });
    const bool passed = bridgeUp && screenReaderActive && allHighlighted && allNamesQueried && !mHiddenSentinelExposed &&
                        mActivateCount > 0 && mAdjustCount > 0 && mToggleCount > 0;

    const char* resultText = mHiddenSentinelExposed ? "RESULT: FAIL" : (passed ? "RESULT: PASS" : "RESULT: WAITING");
    const uint32_t resultColor = mHiddenSentinelExposed ? 0xB03A2Eu : (passed ? 0x196F3Du : 0xA04000u);

    std::ostringstream summary;
    summary << resultText
            << " | highlight " << mHighlightCount[0] << '/' << mHighlightCount[1] << '/' << mHighlightCount[2]
            << " | name query "
            << mPropertyQueryCount[0][0] << '/' << mPropertyQueryCount[1][0] << '/' << mPropertyQueryCount[2][0]
            << " | action " << mActivateCount << '/' << mAdjustCount << '/' << mToggleCount
            << " | hidden " << (mHiddenSentinelExposed ? "FAIL" : "OK");
    mOverall.SetText(summary.str().c_str());
    mOverall.SetAccessibilityName(summary.str().c_str());
    mOverall.SetTextColor(UiColor(resultColor));

    if(reason)
    {
      std::ostringstream log;
      log << reason << "; bridge=" << (bridgeUp ? "UP" : "DOWN")
          << "; screen-reader=" << (screenReaderActive ? "ON" : "OFF")
          << "; result=" << (mHiddenSentinelExposed ? "FAIL" : (passed ? "PASS" : "WAITING"));
      Log(log.str());
    }
    else if(passed && !mPassLogged)
    {
      Log("RESULT=PASS: real bridge highlight and all accessibility actions verified");
    }
    mPassLogged = mPassLogged || passed;
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

  View  mActivate;
  View  mAdjustable;
  View  mToggle;
  Label mActivateLabel;
  Label mAdjustableLabel;
  Label mToggleLabel;
  Label mBridgeStatus;
  Label mLastEvent;
  Label mOverall;

  std::array<int, static_cast<std::size_t>(ControlKind::COUNT)> mHighlightCount{};
  std::array<std::array<int, static_cast<std::size_t>(PropertyKind::COUNT)>,
             static_cast<std::size_t>(ControlKind::COUNT)>      mPropertyQueryCount{};
  int                                                          mActivateCount{0};
  int                                                          mAdjustCount{0};
  int                                                          mToggleCount{0};
  int                                                          mAdjustValue{5};
  bool                                                         mChecked{false};
  bool                                                         mHiddenSentinelExposed{false};
  bool                                                         mPassLogged{false};
};
} // namespace

int DALI_EXPORT_API main(int argc, char** argv)
{
  Application             application = Application::New(&argc, &argv);
  AccessibilityTargetTest test(application);
  application.MainLoop();
  return 0;
}
