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

#include <dali-ui-components/dali-ui-components.h>
#include <dali-ui-foundation/dali-ui-foundation.h>
#include <functional>
#include <sstream>

using namespace Dali;
using namespace Dali::Ui;

class NavigatorExample : public ConnectionTracker
{
public:
  explicit NavigatorExample(Application& application)
  : mApplication(application)
  {
    mApplication.InitSignal().Connect(this, &NavigatorExample::Create);
  }

  void Create(Application application)
  {
    Window window = application.GetWindow();
    window.SetBackgroundColor(Color::WHITE);

    mNavigator = Navigator::New()
                   .SetRequestedWidth(MATCH_PARENT)
                   .SetRequestedHeight(MATCH_PARENT);

    Layout root = Layout::New()
                    .SetRequestedWidth(MATCH_PARENT)
                    .SetRequestedHeight(MATCH_PARENT);

    root.Add(mNavigator);
    root.Add(CreateToolbar());
    window.Add(root);

    mNavigator.Push(CreatePage("Root", UiColor(0xEEF4FF), UiColor(0x2356A6)), false);
    UpdateStatus();
  }

private:
  View CreateToolbar()
  {
    Layout toolbar = Layout::New()
                       .SetLayoutMode(LayoutMode::STANDALONE)
                       .SetRequestedWidth(MATCH_PARENT)
                       .SetRequestedHeight(96_spx)
                       .SetRequestedPositionY(0_spx)
                       .SetBackgroundColor(UiColor(0xFFFFFF));

    toolbar.Add(CreateButton("Push", 16_spx, [this](View, InputEvent) -> bool {
      ++mPageIndex;
      const uint32_t color = (mPageIndex % 2u == 0u) ? 0xFFF8E8 : 0xEAF8EF;
      mNavigator.Push(CreatePage(MakePageTitle(), UiColor(color), UiColor(0x2D6A4F)), true);
      UpdateStatus();
      return true;
    }));

    toolbar.Add(CreateButton("Pop", 136_spx, [this](View, InputEvent) -> bool {
      mNavigator.Pop(true);
      UpdateStatus();
      return true;
    }));

    toolbar.Add(CreateButton("Modal", 256_spx, [this](View, InputEvent) -> bool {
      mNavigator.PushModal(CreateModal(), true);
      UpdateStatus();
      return true;
    }));

    toolbar.Add(CreateButton("Back", 376_spx, [this](View, InputEvent) -> bool {
      mNavigator.NavigateBack();
      UpdateStatus();
      return true;
    }));

    mStatusLabel = Label::New("")
                     .SetRequestedWidth(280_spx)
                     .SetRequestedHeight(48_spx)
                     .SetRequestedPositionX(510_spx)
                     .SetRequestedPositionY(24_spx);
    toolbar.Add(mStatusLabel);

    return toolbar;
  }

  InteractiveView CreateButton(const char* text, float x, std::function<bool(View, InputEvent)> callback)
  {
    InteractiveView button = InteractiveView::New()
                               .SetBackgroundColor(UiColor(0x1D1D1F))
                               .SetRequestedWidth(104_spx)
                               .SetRequestedHeight(48_spx)
                               .SetRequestedPositionX(x)
                               .SetRequestedPositionY(24_spx)
                               .ConnectClickedSignal(this, std::move(callback));

    button.Add(Label::New(text)
                 .SetRequestedWidth(MATCH_PARENT)
                 .SetRequestedHeight(MATCH_PARENT)
                 .SetTextColor(UiColor(0xFFFFFF)));
    return button;
  }

  View CreatePage(const std::string& title, UiColor background, UiColor textColor)
  {
    Layout page = Layout::New()
                    .SetRequestedWidth(MATCH_PARENT)
                    .SetRequestedHeight(MATCH_PARENT)
                    .SetBackgroundColor(background);

    page.Add(Label::New(Dali::String(title.c_str()))
               .SetTextColor(textColor)
               .SetRequestedWidth(500_spx)
               .SetRequestedHeight(80_spx)
               .SetRequestedPositionX(48_spx)
               .SetRequestedPositionY(160_spx));

    page.Add(Label::New("Navigator content area")
               .SetTextColor(UiColor(0x333333))
               .SetRequestedWidth(500_spx)
               .SetRequestedHeight(48_spx)
               .SetRequestedPositionX(48_spx)
               .SetRequestedPositionY(250_spx));

    return page;
  }

  View CreateModal()
  {
    Layout modal = Layout::New()
                    .SetRequestedWidth(MATCH_PARENT)
                    .SetRequestedHeight(MATCH_PARENT)
                    .SetBackgroundColor(UiColor(0x66000000));

    Layout dialog = Layout::New()
                      .SetLayoutMode(LayoutMode::STANDALONE)
                      .SetRequestedWidth(360_spx)
                      .SetRequestedHeight(220_spx)
                      .SetRequestedPositionX(220_spx)
                      .SetRequestedPositionY(190_spx)
                      .SetBackgroundColor(UiColor(0xFFFFFF));

    dialog.Add(Label::New("Modal View")
                 .SetTextColor(UiColor(0x111111))
                 .SetRequestedWidth(300_spx)
                 .SetRequestedHeight(60_spx)
                 .SetRequestedPositionX(28_spx)
                 .SetRequestedPositionY(32_spx));

    dialog.Add(CreateButton("Close", 28_spx, [this](View, InputEvent) -> bool {
      mNavigator.PopModal(true);
      UpdateStatus();
      return true;
    }));

    modal.Add(dialog);
    return modal;
  }

  std::string MakePageTitle() const
  {
    std::ostringstream stream;
    stream << "Page " << mPageIndex;
    return stream.str();
  }

  void UpdateStatus()
  {
    std::ostringstream stream;
    stream << "nav " << mNavigator.GetNavigationStackCount()
           << " / modal " << mNavigator.GetModalStackCount();
    const std::string status = stream.str();
    mStatusLabel.SetText(Dali::String(status.c_str()));
  }

private:
  Application& mApplication;
  Navigator    mNavigator;
  Label        mStatusLabel;
  uint32_t     mPageIndex{0u};
};

int DALI_EXPORT_API main(int argc, char** argv)
{
  Application application = Application::New(&argc, &argv);
  UiConfig::New().Apply();
  NavigatorExample example(application);
  application.MainLoop();
  return 0;
}
