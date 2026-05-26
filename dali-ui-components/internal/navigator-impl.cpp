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
#include <dali-ui-foundation/public-api/focus-manager/focus-manager.h>
#include <dali-ui-foundation/public-api/layouts/layout-types.h>
#include <dali/devel-api/actors/actor-devel.h>
#include <dali/devel-api/adaptor-framework/window-devel.h>
#include <dali/devel-api/object/property-helper-devel.h>
#include <dali/public-api/actors/actor.h>
#include <dali/public-api/adaptor-framework/window.h>
#include <dali/public-api/animation/alpha-function.h>
#include <dali/public-api/object/property.h>
#include <dali/public-api/render-tasks/render-task-list.h>
#include <algorithm>
#include <stdexcept>

// INTERNAL INCLUDES
#include <dali-ui-components/internal/navigator-impl.h>

namespace Dali
{

namespace Ui
{

namespace Internal
{

namespace
{
constexpr unsigned int NAVIGATION_POSITION_DURATION = 450u;
constexpr unsigned int NAVIGATION_OPACITY_DURATION  = 150u;
constexpr unsigned int MODAL_DURATION               = 300u;

float GetWindowWidth(Dali::Actor actor)
{
  Window window = DevelWindow::Get(actor);
  if(window)
  {
    return window.GetSize().GetWidth();
  }
  return 720.0f;
}
} // namespace

Ui::Navigator NavigatorImpl::New()
{
  IntrusivePtr<Internal::NavigatorImpl> impl = new Internal::NavigatorImpl();

  Ui::Navigator handle = Ui::Navigator(*impl);

  impl->Initialize();

  return handle;
}

NavigatorImpl::NavigatorImpl()
: ViewImpl()
{
}

NavigatorImpl::~NavigatorImpl()
{
  StopCurrentAnimation();
  mNavigationStack.clear();
  mModalStack.clear();
  mSavedProperties.clear();
}

void NavigatorImpl::OnInitialize()
{
  ViewImpl::OnInitialize();
  SetRequestedWidth(MATCH_PARENT);
  SetRequestedHeight(MATCH_PARENT);
}

uint32_t NavigatorImpl::GetNavigationStackCount() const
{
  return static_cast<uint32_t>(mNavigationStack.size());
}

uint32_t NavigatorImpl::GetModalStackCount() const
{
  return static_cast<uint32_t>(mModalStack.size());
}

Ui::View NavigatorImpl::GetNavigationStackAt(uint32_t index) const
{
  return index < mNavigationStack.size() ? mNavigationStack[index] : Ui::View();
}

Ui::View NavigatorImpl::GetModalStackAt(uint32_t index) const
{
  return index < mModalStack.size() ? mModalStack[index] : Ui::View();
}

Ui::View NavigatorImpl::GetCurrentView() const
{
  Ui::View modalTop = GetTopView(mModalStack);
  return modalTop ? modalTop : GetTopView(mNavigationStack);
}

void NavigatorImpl::Push(Ui::View view, bool animated)
{
  ThrowIfInvalidOrContained(view);
  StopCurrentAnimation();

  Ui::View previousTop = GetTopView(mNavigationStack);
  mNavigationStack.push_back(view);
  Self().Add(view);

  if(!mModalStack.empty())
  {
    view.LowerBelow(mModalStack.front(), LayoutOrderPolicy::PRESERVE);
    SaveNavigationProperties(view);
  }

  if(previousTop)
  {
    SaveNavigationProperties(previousTop);
  }

  Ui::FocusManager::Get().ClearFocus();
  PlayDefaultTransition(view, previousTop, animated && previousTop, false);

  if(previousTop && previousTop != GetTopView(mNavigationStack))
  {
    SetHiddenBelowTop(previousTop, true);
  }

  view.SetVisibility(true);
}

void NavigatorImpl::PushModal(Ui::View view, bool animated)
{
  ThrowIfInvalidOrContained(view);
  StopCurrentAnimation();

  Ui::View previousTop = GetCurrentView();
  mModalStack.push_back(view);
  Self().Add(view);

  if(previousTop)
  {
    SaveNavigationProperties(previousTop);
  }

  Ui::FocusManager::Get().ClearFocus();
  PlayDefaultTransition(view, previousTop, animated, true);
  view.SetVisibility(true);
}

Ui::View NavigatorImpl::Pop(bool animated)
{
  StopCurrentAnimation();

  Ui::View popped = GetTopView(mNavigationStack);
  if(!popped)
  {
    return Ui::View();
  }

  mNavigationStack.pop_back();

  Ui::View newTop = GetTopView(mNavigationStack);
  if(newTop)
  {
    SetHiddenBelowTop(newTop, false);
  }

  PlayDefaultTransition(newTop, popped, animated && newTop, false);

  if(newTop)
  {
    LoadNavigationProperties(newTop);
  }

  LoadNavigationProperties(popped);
  Self().Remove(popped);

  return popped;
}

Ui::View NavigatorImpl::PopModal(bool animated)
{
  StopCurrentAnimation();

  Ui::View popped = GetTopView(mModalStack);
  if(!popped)
  {
    return Ui::View();
  }

  mModalStack.pop_back();

  Ui::View newTop = GetCurrentView();
  PlayDefaultTransition(newTop, popped, animated, true);

  if(newTop)
  {
    LoadNavigationProperties(newTop);
  }

  LoadNavigationProperties(popped);
  Self().Remove(popped);

  return popped;
}

void NavigatorImpl::InsertBefore(Ui::View view, Ui::View before)
{
  ThrowIfInvalidOrContained(view);
  auto beforeIt = Find(mNavigationStack, before);
  if(beforeIt == mNavigationStack.end())
  {
    throw std::invalid_argument("The before view does not exist in the navigation stack.");
  }

  Self().Add(view);
  view.LowerBelow(before, LayoutOrderPolicy::PRESERVE);
  mNavigationStack.insert(beforeIt, view);

  SaveNavigationProperties(view);
  SetHiddenBelowTop(view, true);
}

void NavigatorImpl::Remove(Ui::View view)
{
  ThrowIfMissing(view);

  if(view == GetTopView(mNavigationStack))
  {
    Pop(false);
  }
  else if(view == GetTopView(mModalStack))
  {
    PopModal(false);
  }
  else
  {
    LoadNavigationProperties(view);
    RemoveInternal(view);
  }
}

void NavigatorImpl::Clear()
{
  StopCurrentAnimation();

  while(!mModalStack.empty())
  {
    Ui::View view = mModalStack.back();
    mModalStack.pop_back();
    LoadNavigationProperties(view);
    Self().Remove(view);
  }

  while(!mNavigationStack.empty())
  {
    Ui::View view = mNavigationStack.back();
    mNavigationStack.pop_back();
    LoadNavigationProperties(view);
    Self().Remove(view);
  }

  mSavedProperties.clear();
}

bool NavigatorImpl::NavigateBack()
{
  if(GetTopView(mModalStack))
  {
    PopModal(true);
    return true;
  }

  if(mNavigationStack.size() > 1u)
  {
    Pop(true);
    return true;
  }

  return false;
}

bool NavigatorImpl::Contains(Ui::View view) const
{
  return Contains(mNavigationStack, view) || Contains(mModalStack, view);
}

bool NavigatorImpl::Contains(const Stack& stack, Ui::View view) const
{
  return Find(stack, view) != stack.end();
}

NavigatorImpl::Stack::iterator NavigatorImpl::Find(Stack& stack, Ui::View view)
{
  return std::find(stack.begin(), stack.end(), view);
}

NavigatorImpl::Stack::const_iterator NavigatorImpl::Find(const Stack& stack, Ui::View view) const
{
  return std::find(stack.begin(), stack.end(), view);
}

Ui::View NavigatorImpl::GetTopView(const Stack& stack) const
{
  return stack.empty() ? Ui::View() : stack.back();
}

void NavigatorImpl::ThrowIfInvalidOrContained(Ui::View view) const
{
  if(!view)
  {
    throw std::invalid_argument("Navigator requires an initialized View.");
  }

  if(Contains(view))
  {
    throw std::invalid_argument("The view already exists in the navigator stack.");
  }
}

void NavigatorImpl::ThrowIfMissing(Ui::View view) const
{
  if(!view)
  {
    throw std::invalid_argument("Navigator requires an initialized View.");
  }

  if(!Contains(view))
  {
    throw std::invalid_argument("The view does not exist in the navigator stack.");
  }
}

void NavigatorImpl::SaveNavigationProperties(Ui::View view)
{
  if(!view)
  {
    return;
  }

  auto it = std::find_if(mSavedProperties.begin(), mSavedProperties.end(), [view](const auto& item)
  {
    return item.first == view;
  });
  if(it == mSavedProperties.end())
  {
    mSavedProperties.push_back({view, {view.IsEnabled(), view.IsFocusable(), view.IsDescendantFocusBlocked()}});
  }

  view.SetEnabled(false);
  view.SetFocusable(false);
  view.SetDescendantFocusBlocked(true);
}

void NavigatorImpl::LoadNavigationProperties(Ui::View view)
{
  if(!view)
  {
    return;
  }

  auto it = std::find_if(mSavedProperties.begin(), mSavedProperties.end(), [view](const auto& item)
  {
    return item.first == view;
  });
  if(it == mSavedProperties.end())
  {
    return;
  }

  view.SetEnabled(it->second.enabled);
  view.SetFocusable(it->second.focusable);
  view.SetDescendantFocusBlocked(it->second.descendantFocusBlocked);
  mSavedProperties.erase(it);
}

void NavigatorImpl::RemoveInternal(Ui::View view)
{
  auto navIt = Find(mNavigationStack, view);
  if(navIt != mNavigationStack.end())
  {
    mNavigationStack.erase(navIt);
    Ui::View newTop = GetTopView(mNavigationStack);
    if(newTop)
    {
      SetHiddenBelowTop(newTop, false);
    }
  }

  auto modalIt = Find(mModalStack, view);
  if(modalIt != mModalStack.end())
  {
    mModalStack.erase(modalIt);
  }

  Self().Remove(view);
}

void NavigatorImpl::SetHiddenBelowTop(Ui::View view, bool hidden)
{
  if(!view)
  {
    return;
  }

  view.SetVisibility(!hidden);
  view.SetProperty(Ui::View::Property::ACCESSIBILITY_HIDDEN, hidden);
}

void NavigatorImpl::PlayDefaultTransition(Ui::View appearing, Ui::View disappearing, bool animated, bool modal)
{
  if(!animated)
  {
    if(appearing)
    {
      appearing.SetOpacity(1.0f);
    }
    if(disappearing)
    {
      disappearing.SetOpacity(1.0f);
    }
    return;
  }

  if(appearing)
  {
    appearing.SetVisibility(true);
    appearing.SetOpacity(0.0f);
  }

  mCurrentAnimation = Animation::New(modal ? MODAL_DURATION : NAVIGATION_POSITION_DURATION);
  AlphaFunction alpha(AlphaFunction::EASE_OUT);

  if(appearing)
  {
    mCurrentAnimation.AnimateTo(Property(appearing, Actor::Property::OPACITY), 1.0f, alpha, TimePeriod(0u, NAVIGATION_OPACITY_DURATION));
  }

  if(disappearing)
  {
    mCurrentAnimation.AnimateTo(Property(disappearing, Actor::Property::OPACITY), modal ? 1.0f : 0.0f, alpha, TimePeriod(0u, NAVIGATION_OPACITY_DURATION));
  }

  if(!modal)
  {
    const float offset = GetWindowWidth(Self()) * 0.1f;
    if(appearing)
    {
      const float finalX = appearing.GetPositionX();
      appearing.SetProperty(Actor::Property::POSITION_X, finalX + offset);
      mCurrentAnimation.AnimateTo(Property(appearing, Actor::Property::POSITION_X), finalX, alpha);
    }
    if(disappearing)
    {
      const float finalX = disappearing.GetPositionX();
      mCurrentAnimation.AnimateTo(Property(disappearing, Actor::Property::POSITION_X), finalX - offset, alpha);
    }
  }

  mCurrentAnimation.Play();
}

void NavigatorImpl::StopCurrentAnimation()
{
  if(mCurrentAnimation)
  {
    mCurrentAnimation.Stop();
    mCurrentAnimation.Reset();
    mCurrentAnimation = Animation();
  }
}

} // namespace Internal

} // namespace Ui

} // namespace Dali
