# PR Description: Add Dali::Ui::Navigator

## Summary

This PR adds `Dali::Ui::Navigator`, a stack-based UI container that mirrors the
core behavior of the OneUI C# `Navigator` component in dali-ui C++ style.

Navigator manages:

- A regular navigation stack for application pages.
- A modal stack for dialog-like content shown above regular pages.
- Current view resolution, where modal views take priority.
- Back navigation with modal-first behavior.
- Visibility, enabled state, and descendant focus blocking for views below the
  current top view.

## Public API

New public component:

- `dali-ui-components/public-api/navigator.h`

Main APIs:

- `Navigator::New()`
- `Push(View, bool animated = true)`
- `PushModal(View, bool animated = true)`
- `Pop(bool animated = true)`
- `PopModal(bool animated = true)`
- `InsertBefore(View, View before)`
- `Remove(View)`
- `Clear()`
- `NavigateBack()`
- Stack inspection APIs for navigation/modal counts and indexed access.

## Implementation

The implementation follows the existing dali-ui component pattern:

- Public handle: `Dali::Ui::Navigator`
- Internal implementation: `Dali::Ui::Internal::NavigatorImpl`
- Base class: `View`
- Internal state stored in `NavigatorImpl`

The first implementation keeps the API synchronous. It preserves the C# stack
semantics while using C++ handle APIs and simple default transitions.

## Tests

Added `utc-Dali-Navigator.cpp` with coverage for:

- Constructor/New/copy/move/assignment/DownCast
- Push and Pop
- PushModal and PopModal
- InsertBefore
- Remove
- Clear
- NavigateBack
- Invalid operations

Expected test target:

```sh
tct-dali-ui-components-core
```

## Sample

Added sample app:

```sh
samples/navigator/navigator-example.cpp
```

The sample demonstrates page push/pop, modal push/pop, and back navigation.

## Notes

The OneUI C# `Navigator` exposes async transition controllers and per-view
custom animation interfaces. This PR keeps those as future extension points so
the initial C++ API remains small and idiomatic for dali-ui.
