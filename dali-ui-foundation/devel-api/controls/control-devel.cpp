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

// CLASS HEADER
#include <dali-ui-foundation/devel-api/controls/control-devel.h>

// INTERNAL INCLUDES
#include <controls/control/control-data-impl.h>

namespace Dali
{
namespace Ui
{
namespace DevelControl
{

void RegisterVisual(Internal::Control& control, Dali::Property::Index index, Ui::Visual::Base& visual)
{
  Internal::Control::Impl::Get(control).RegisterVisual(index, visual);
}

void RegisterVisual(Internal::Control& control, Dali::Property::Index index, Ui::Visual::Base& visual, int depthIndex)
{
  Internal::Control::Impl::Get(control).RegisterVisual(index, visual, depthIndex);
}

void RegisterVisual(Internal::Control& control, Dali::Property::Index index, Ui::Visual::Base& visual, bool enabled)
{
  Internal::Control::Impl::Get(control).RegisterVisual(index, visual, enabled);
}

void RegisterVisual(Internal::Control& control, Dali::Property::Index index, Ui::Visual::Base& visual, bool enabled, int depthIndex)
{
  Internal::Control::Impl::Get(control).RegisterVisual(index, visual, enabled, depthIndex);
}

void UnregisterVisual(Internal::Control& control, Dali::Property::Index index)
{
  Internal::Control::Impl::Get(control).UnregisterVisual(index);
}

void EnableCornerPropertiesOverridden(Internal::Control& control, Visual::Base& visual, bool enable, Dali::Constraint cornerRadiusConstraint)
{
  Internal::Control::Impl::Get(control).EnableCornerPropertiesOverridden(visual, enable, cornerRadiusConstraint);
}

} // namespace DevelControl
} // namespace Ui
} // namespace Dali
