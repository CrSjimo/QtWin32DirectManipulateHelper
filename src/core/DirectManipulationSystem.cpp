#include "DirectManipulationSystem.h"
#include "DirectManipulationSystem_p.h"

#include <QGuiApplication>
#include <QWindow>
#include <QtEvents>
#include <QPointer>
#include <QTimer>
#include <utility>

namespace QWDMH {

    static DirectManipulationSystem *m_instance = nullptr;

    class DirectManipulationSystemEventHandler : public Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>, IDirectManipulationViewportEventHandler> {
    public:
        explicit DirectManipulationSystemEventHandler(QWindow *window, Microsoft::WRL::ComPtr<IDirectManipulationUpdateManager> updateManager) : m_window(window), m_updateManager(std::move(updateManager)) {
            m_inertiaTimer.setInterval(0);
            m_inertiaTimer.callOnTimeout([=] {
                m_updateManager->Update(nullptr);
            });
        }

        inline void sendZoomEvent(float scale) const {
            auto pos = QCursor::pos();
            auto localPos = m_window->mapFromGlobal(pos);
            auto event = new QNativeGestureEvent(Qt::ZoomNativeGesture, QPointingDevice::primaryPointingDevice(), 2, localPos, localPos, pos, scale / previousScale - 1, {0, 0});
            QCoreApplication::sendEvent(m_window, event);
        }

        inline void sendWheelEvent(int dx, int dy) const {
            auto pos = QCursor::pos();
            auto localPos = m_window->mapFromGlobal(pos);
            auto modifiers = QGuiApplication::queryKeyboardModifiers();
            QPoint delta = {dx, dy};
            if (modifiers & Qt::AltModifier) {
                delta = delta.transposed();
            }
            auto event = new QWheelEvent(localPos, pos, delta, delta, QGuiApplication::mouseButtons(), modifiers, Qt::ScrollUpdate, false);
            QCoreApplication::sendEvent(m_window, event);
        }

        STDMETHODIMP OnContentUpdated(IDirectManipulationViewport*, IDirectManipulationContent* content) override {
            float matrix[6];
            if (SUCCEEDED(content->GetContentTransform(matrix, 6))) {
                float scale = matrix[0];
                float tx = matrix[4];
                float ty = matrix[5];

                if (!qFuzzyCompare(scale, previousScale)) {
                    sendZoomEvent(scale);
                } else {
                    auto dx = std::round(tx - previousTX);
                    auto dy = std::round(ty - previousTY);
                    tx = dx + previousTX;
                    ty = dy + previousTY;
                    sendWheelEvent(dx, dy);
                }
                previousScale = scale;
                previousTX = tx;
                previousTY = ty;
            }
            return S_OK;
        }

        STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override {
            if (riid == __uuidof(IDirectManipulationViewportEventHandler) || riid == __uuidof(IUnknown)) {
                *ppv = static_cast<IDirectManipulationViewportEventHandler*>(this);
                AddRef();
                return S_OK;
            }
            *ppv = nullptr;
            return E_NOINTERFACE;
        }
        STDMETHODIMP OnViewportStatusChanged(IDirectManipulationViewport* viewport, DIRECTMANIPULATION_STATUS current, DIRECTMANIPULATION_STATUS previous) override {
            if (current == DIRECTMANIPULATION_RUNNING) {
                m_inertiaTimer.stop();
                auto pos = QCursor::pos();
                auto localPos = m_window->mapFromGlobal(pos);
                auto event = new QNativeGestureEvent(Qt::BeginNativeGesture, QPointingDevice::primaryPointingDevice(), 2, localPos, localPos, pos, 0, {0, 0});
                QCoreApplication::sendEvent(m_window, event);
            } else if (current == DIRECTMANIPULATION_INERTIA) {
                m_inertiaTimer.start();
            } else if (current == DIRECTMANIPULATION_READY) {
                m_inertiaTimer.stop();
                IDirectManipulationContent *content;
                viewport->GetPrimaryContent(IID_PPV_ARGS(&content));
                float matrix[] = {1, 0, 0, 1, 0, 0};
                content->SyncContentTransform(matrix, 6);
                previousScale = 1;
                previousTX = previousTY = 0;
                auto pos = QCursor::pos();
                auto localPos = m_window->mapFromGlobal(pos);
                auto event = new QNativeGestureEvent(Qt::EndNativeGesture, QPointingDevice::primaryPointingDevice(), 2, localPos, localPos, pos, 0, {0, 0});
                QCoreApplication::sendEvent(m_window, event);
            }
            return S_OK;
        }
        STDMETHODIMP OnViewportUpdated(IDirectManipulationViewport*) override { return S_OK; }

    private:
        QPointer<QWindow> m_window;
        Microsoft::WRL::ComPtr<IDirectManipulationUpdateManager> m_updateManager;
        QTimer m_inertiaTimer;
        float previousScale = 1;
        float previousTX = 0;
        float previousTY = 0;
    };

    bool DirectManipulationSystemPrivate::EventFilter::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) {
        auto msg = static_cast<MSG*>(message);
        if (shouldProcessMessage(msg)) {
            return DirectManipulationSystem::processNativeMessageManually(msg);
        }
        return false;
    }

    DirectManipulationSystemPrivate* DirectManipulationSystemPrivate::instance() {
        return m_instance ? m_instance->d_func() : nullptr;
    }

    bool DirectManipulationSystemPrivate::shouldProcessMessage(MSG* msg) {
        auto d = m_instance->d_func();
        if (!d->contexts.contains(msg->hwnd))
            return false;
        auto context = d->contexts.value(msg->hwnd);
        if ((msg->message == WM_MOUSEWHEEL || msg->message == WM_MOUSEHWHEEL) && context.deviceType & DirectManipulationSystem::Wheel)
            return true;
        if (msg->message == WM_POINTERUPDATE || msg->message == DM_POINTERHITTEST) {
            UINT32 pointerId = GET_POINTERID_WPARAM(msg->wParam);
            POINTER_INFO info{};
            if (!GetPointerInfo(pointerId, &info))
                return false;
            return
                info.pointerType == PT_TOUCHPAD && context.deviceType & DirectManipulationSystem::Touchpad ||
                info.pointerType == PT_PEN && context.deviceType & DirectManipulationSystem::Pen ||
                info.pointerType == PT_TOUCH && context.deviceType & DirectManipulationSystem::Touch;
        }
        return false;
    }

    DirectManipulationSystem::DirectManipulationSystem(QObject* parent) : QObject(parent), d_ptr(new DirectManipulationSystemPrivate) {
        Q_D(DirectManipulationSystem);
        d->q_ptr = m_instance = this;
        QCoreApplication::instance()->installNativeEventFilter(&d->eventFilter);
    }

    DirectManipulationSystem::~DirectManipulationSystem() {
        Q_D(DirectManipulationSystem);
        m_instance = nullptr;
        QCoreApplication::instance()->removeNativeEventFilter(&d->eventFilter);
    }

    DirectManipulationSystem* DirectManipulationSystem::instance() {
        return m_instance;
    }

    void DirectManipulationSystem::registerWindow(QWindow* window) {
        registerWindow(window, TranslationX | TranslationY | Scaling | TranslationInertia | ScalingInertia, All);
    }

    void DirectManipulationSystem::registerWindow(QWindow* window, Configuration configuration, DeviceType deviceType) {
        Q_ASSERT(m_instance);
        auto d = m_instance->d_func();
        auto hwnd = reinterpret_cast<HWND>(window->winId());
        if (d->contexts.contains(hwnd)) return;

        DirectManipulationSystemPrivate::ViewportContext context{.deviceType = deviceType};

        if (FAILED(CoCreateInstance(CLSID_DirectManipulationManager, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&context.manager))))
            return;

        if (FAILED(context.manager->Activate(hwnd)) || FAILED(context.manager->CreateViewport(nullptr, hwnd, IID_PPV_ARGS(&context.viewport)))) {
            return;
        }

        context.manager->GetUpdateManager(IID_PPV_ARGS(&context.updateManager));

        context.viewport->ActivateConfiguration(DIRECTMANIPULATION_CONFIGURATION_INTERACTION | static_cast<DIRECTMANIPULATION_CONFIGURATION>(configuration.toInt()));

        RECT rc;
        GetClientRect(hwnd, &rc);
        context.viewport->SetViewportRect(&rc);

        auto handler = Microsoft::WRL::Make<DirectManipulationSystemEventHandler>(window, context.updateManager);
        context.viewport->AddEventHandler(hwnd, handler.Get(), &context.eventCookie);
        context.viewport->Enable();

        d->contexts.insert(hwnd, context);
        d->winIds.insert(window, hwnd);

        connect(window, &QObject::destroyed, m_instance, [=] {
            unregisterWindow(window);
        });
    }

    void DirectManipulationSystem::unregisterWindow(QWindow* window) {
        Q_ASSERT(m_instance);
        auto d = m_instance->d_func();
        if (!d->winIds.contains(window))
            return;
        auto hwnd = d->winIds.value(window);
        auto context = d->contexts.value(hwnd);
        context.viewport->RemoveEventHandler(context.eventCookie);
        context.viewport->Disable();
        d->contexts.remove(hwnd);
        d->winIds.remove(window);
    }

    bool DirectManipulationSystem::processNativeMessageManually(void *message) {
        Q_ASSERT(m_instance);
        auto d = m_instance->d_func();
        auto msg = static_cast<MSG*>(message);
        HWND hwnd = msg->hwnd;
        UINT32 pointerId;
        BOOL fHandled = FALSE;
        switch (msg->message) {
            case WM_POINTERUPDATE:
            case DM_POINTERHITTEST:
                pointerId = GET_POINTERID_WPARAM(msg->wParam);
                if (d->contexts.contains(hwnd)) {
                    auto context = d->contexts.value(hwnd);
                    context.viewport->SetContact(pointerId);
                    context.updateManager->Update(nullptr);
                    return true;
                }
                break;
            case WM_MOUSEWHEEL:
            case WM_MOUSEHWHEEL:
                pointerId = DIRECTMANIPULATION_MOUSEFOCUS;
                if (d->contexts.contains(hwnd)) {
                    auto context = m_instance->d_func()->contexts.value(hwnd);
                    context.viewport->SetContact(pointerId);
                    MSG msg_ = *msg;
                    msg_.wParam &= 0xffff0000;
                    context.manager->ProcessInput(&msg_, &fHandled);
                    context.viewport->ReleaseContact(pointerId);
                    context.updateManager->Update(nullptr);
                    return true;
                }
                break;
            default:
                break;
        }
        return false;
    }
}
