# Navigator

`Dali::Ui::Navigator` is a stack-based container for view navigation.
It owns two stacks:

- Navigation stack: regular application pages.
- Modal stack: modal content shown above the navigation stack.

The current view is the top modal view when a modal exists; otherwise it is the
top navigation view.

## Basic Usage

```cpp
#include <dali-ui-components/dali-ui-components.h>

using namespace Dali;
using namespace Dali::Ui;

Navigator navigator = Navigator::New()
                        .SetRequestedWidth(MATCH_PARENT)
                        .SetRequestedHeight(MATCH_PARENT);

View root = View::New();
View page = View::New();

navigator.Push(root, false);
navigator.Push(page, true);
navigator.Pop(true);
```

## Modal Usage

```cpp
View dialogContainer = View::New();

navigator.PushModal(dialogContainer, true);
navigator.PopModal(true);
```

Modal views are added above navigation views. While a modal is active, the
previous current view is disabled and descendant focus is blocked. Those states
are restored when the modal is popped or removed.

## Back Navigation

```cpp
bool handled = navigator.NavigateBack();
```

`NavigateBack()` pops a modal first. If there is no modal, it pops the
navigation stack until only the root view remains. It returns `false` when no
navigation action is available.

## Child Management

Use `Push()`, `PushModal()`, `Pop()`, `PopModal()`, `InsertBefore()`,
`Remove()`, and `Clear()` to manage children. Direct child insertion with
`Add()` bypasses Navigator stack bookkeeping and should not be used for
Navigator-managed views.

## Sample

Run the sample app from:

```sh
samples/navigator/navigator-example.cpp
```

The sample demonstrates regular push/pop, modal push/pop, and back navigation.

## Current Limitations

- Default transitions are intentionally simple and are implemented inside
  Navigator.
- Per-view custom transition controllers from the C# OneUI Navigator are not
  exposed yet. A future extension can add callback or signal based transition
  hooks without changing the core stack API.
