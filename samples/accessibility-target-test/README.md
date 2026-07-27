# Accessibility Target Test

`accessibility-target-test.example` verifies the accessibility path that the
existing `AccessibilityTest` cannot cover by itself:

```text
real Screen Reader -> AT-SPI / D-Bus bridge -> DALi ViewAccessible
                   -> ViewImpl accessibility callback -> sample UI and log
```

The existing sample uses `Accessible::Get()` and invokes an action inside the
same process. This target sample deliberately does neither. Its counters change
only when an external accessibility client highlights a control or invokes an
exported action.

## Why the existing sample can pass while a target fails

The relevant runtime path in this repository is:

```text
View::SetAccessibility*()
  -> ViewDataImpl::AccessibilityData
  -> ViewAccessible (name, role, state, attributes)
  -> adaptor Accessibility::Bridge
  -> target AT-SPI / D-Bus accessibility service
  -> Screen Reader
```

The return action takes the reverse path:

```text
Screen Reader action
  -> AT-SPI Action.DoAction
  -> ViewAccessible::DoAction()
  -> View::DoAction("activate" | "increment" | "decrement")
  -> ViewImpl::OnAccessibilityActivate/OnAccessibilityValueChange
```

`samples/AccessibilityTest` starts in the middle of these paths. It obtains the
in-process `Accessible` and invokes its action directly, so it proves the View
metadata conversion and action dispatch but not bridge activation, D-Bus object
export, target accessibility-service connection, Screen Reader discovery, or
highlight delivery.

This sample observes the missing boundaries explicitly:

- `Accessibility::Bridge::{Enabled,Disabled}Signal()` and Screen Reader status
  signals show service transitions while the app is running.
- `View::AccessibilityHighlightedSignal()` proves that the active bridge
  selected a concrete DALi View.
- `ViewImpl::OnAccessibilityRequestName/Description/Value()` counters prove
  that an external client queried the exported object.
- `ViewImpl::OnAccessibilityActivate/OnAccessibilityValueChange()` counters
  prove that external actions returned through the bridge.
- `Accessibility::Say()` exercises the outbound DALi-to-Screen-Reader path
  after the activation callback arrives.

## What constitutes PASS

The result changes to `PASS` only after all of the following are true:

- The DALi AT-SPI bridge reports `UP`.
- Screen Reader reports `ON`.
- The real client highlights all three test controls.
- The real client queries the exported name of all three controls through the
  `ViewImpl::OnAccessibilityRequestName()` virtual path.
- The button receives an `activate` callback.
- The adjustable control receives an `increment` or `decrement` callback.
- The checkbox receives an `activate` callback and changes its checked state.

The app also keeps an accessibility-hidden sentinel on scene. Its name starts
with `FAIL:`; hearing that text or seeing `hidden FAIL` means the target bridge
queried an object that should have been filtered. The three visible controls
additionally expose collection indices, automation IDs, and `DESCRIBED_BY`
relations to the instruction text.

Name, description, and value virtual callbacks are counted and logged. Reading
lifecycle callbacks and the `Say()` callback are also printed when the target
Screen Reader provides them, but they are not required for PASS because their
availability depends on the Screen Reader implementation.

## Desktop build

From this directory:

```sh
cmake --fresh -DCMAKE_INSTALL_PREFIX=$DESKTOP_PREFIX .
make -j8
./bin/accessibility-target-test.example
```

A desktop run is useful for layout and bridge diagnostics. It is not a real
target PASS unless an external Screen Reader is connected.

## Tizen target package

From the `dali-ui` repository root:

```sh
gbs build -A armv7l --include-all \
  --packaging-dir samples/accessibility-target-test/packaging
```

Deploy the generated RPM with the normal image/RPM workflow, then launch:

```sh
app_launcher -s com.samsung.dali.accessibility-target-test
```

## Target verification procedure

1. Enable Screen Reader (or Voice Guide on a TV profile).
2. Start `DALi Accessibility Target Test`.
3. Confirm the status says `AT-SPI bridge: UP` and `Screen Reader: ON`.
4. Navigate until each colored test control receives the accessibility
   highlight.
5. Double-tap/activate `Activation test button`.
6. Use the Screen Reader increase or decrease gesture on
   `Adjustable test value`.
7. Double-tap/activate `Toggle test checkbox`.
8. Confirm the last line changes to `RESULT: PASS`.

The counters are ordered as `activate / adjustable / checkbox`. The highlight,
name-query, and action counters use the same order. Target logs use the
`[A11Y_TARGET]` prefix:

```sh
sdb dlog | grep A11Y_TARGET
```

Expected key lines include:

```text
[A11Y_TARGET] application initialized; bridge=UP; screen-reader=ON; result=WAITING
[A11Y_TARGET] external property query: activate button name
[A11Y_TARGET] highlight received for activate button
[A11Y_TARGET] activate callback count=1
[A11Y_TARGET] increment callback value=6
[A11Y_TARGET] toggle callback checked=true
[A11Y_TARGET] RESULT=PASS: real bridge highlight and all accessibility actions verified
```

If the bridge remains `DOWN`, the issue is below the DALi UI View API: check the
target accessibility service, AT-SPI/D-Bus availability, DALi adaptor build
options, and whether the application was started after the accessibility
service became available.
