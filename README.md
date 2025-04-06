# QtWin32DirectManipulateHelper

A Qt integration of Win32 Direct Manipulate APIs

## Usage

To enable Direct Manipulation on a `QWindow`, call

```c++
QWDMH::DirectManipulationSystem::registerWindow(window);
```

To disable, call

```c++
QWDMH::DirectManipulationSystem::unregisterWindow(window);
```

After Direct Manipulation is enabled on a `QWindow`, a `QNativeGestureEvent` with `QNativeGestureEvent::gestureType()` to be `Qt::ZoomNativeGesture` will be sent when the user pinches on the touchpad, and a `QWheelEvent` will be sent when the user swipes on the touchpad with two fingers.

It is also supported to specify configuration and device type when registering a window, for example

```c++
QWDMH::DirectManipulationSystem::registerWindow(
    window,
    QWDMH::DirectManipulationSystem::TranslationX | QWDMH::DirectManipulationSystem::TranslationY,
    QWDMH::DirectManipulationSystem::Touchpad);
```

The example above makes the window processes only translation without inertia, and accepts input only from the touchpad.