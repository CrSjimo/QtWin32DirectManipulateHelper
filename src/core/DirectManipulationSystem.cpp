#include "DirectManipulationSystem.h"
#include "DirectManipulationSystem_p.h"

#include <QCoreApplication>
#include <QWindow>
#include <QtEvents>
#include <QPointer>

namespace QWDMH {

    static DirectManipulationSystem *m_instance = nullptr;

    class DirectManipulationSystemEventHandler : public Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>, IDirectManipulationViewportEventHandler> {
    public:
        explicit DirectManipulationSystemEventHandler(QWindow *window) : m_window(window) {}

        STDMETHODIMP OnContentUpdated(IDirectManipulationViewport*, IDirectManipulationContent* content) override {
            float matrix[6];
            if (SUCCEEDED(content->GetContentTransform(matrix, 6))) {
                float scale = matrix[0];
                float tx = matrix[4];
                float ty = matrix[5];
                auto pos = QCursor::pos();
                auto localPos = m_window->mapFromGlobal(pos);
                if (!qFuzzyCompare(scale, previousScale)) {
                    auto event = new QNativeGestureEvent(Qt::ZoomNativeGesture, QPointingDevice::primaryPointingDevice(), 2, localPos, localPos, pos, scale / previousScale - 1, {0, 0});
                    QCoreApplication::sendEvent(m_window, event);
                } else {
                    auto event = new QWheelEvent(localPos, pos, {static_cast<int>(tx - previousTX), static_cast<int>(ty - previousTY)}, {static_cast<int>(tx - previousTX), static_cast<int>(ty - previousTY)}, Qt::NoButton, Qt::NoModifier, Qt::ScrollUpdate, false);
                    QCoreApplication::sendEvent(m_window, event);
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
                auto pos = QCursor::pos();
                auto localPos = m_window->mapFromGlobal(pos);
                auto event = new QNativeGestureEvent(Qt::BeginNativeGesture, QPointingDevice::primaryPointingDevice(), 2, localPos, localPos, pos, 0, {0, 0});
                QCoreApplication::sendEvent(m_window, event);
            } else if (current == DIRECTMANIPULATION_READY) {
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
        float previousScale = 1;
        float previousTX = 0;
        float previousTY = 0;
    };

    bool DirectManipulationSystemPrivate::EventFilter::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) {
        auto msg = static_cast<MSG*>(message);
        if (msg->message == DM_POINTERHITTEST) {
            HWND hwnd = msg->hwnd;
            UINT32 pointerId = GET_POINTERID_WPARAM(msg->wParam);
            handleContact(hwnd, pointerId);
            return true;
        }
        return false;
    }

    DirectManipulationSystemPrivate* DirectManipulationSystemPrivate::instance() {
        return m_instance ? m_instance->d_func() : nullptr;
    }

    void DirectManipulationSystemPrivate::handleContact(HWND hwnd, UINT32 pointerId) {
        if (m_instance->d_func()->contexts.contains(hwnd)) {
            auto context = m_instance->d_func()->contexts.value(hwnd);
            context.viewport->SetContact(pointerId);
            context.updateManager->Update(nullptr);
        }
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
        Q_ASSERT(m_instance);
        auto d = m_instance->d_func();
        auto hwnd = reinterpret_cast<HWND>(window->winId());
        if (d->contexts.contains(hwnd)) return;

        DirectManipulationSystemPrivate::ViewportContext context;

        if (FAILED(CoCreateInstance(CLSID_DirectManipulationManager, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&context.manager))))
            return;

        if (FAILED(context.manager->Activate(hwnd)) || FAILED(context.manager->CreateViewport(nullptr, hwnd, IID_PPV_ARGS(&context.viewport)))) {
            return;
        }

        context.manager->GetUpdateManager(IID_PPV_ARGS(&context.updateManager));

        context.viewport->ActivateConfiguration(
            DIRECTMANIPULATION_CONFIGURATION_INTERACTION |
            DIRECTMANIPULATION_CONFIGURATION_SCALING |
            DIRECTMANIPULATION_CONFIGURATION_TRANSLATION_X |
            DIRECTMANIPULATION_CONFIGURATION_TRANSLATION_Y);

        RECT rc;
        GetClientRect(hwnd, &rc);
        context.viewport->SetViewportRect(&rc);

        auto handler = Microsoft::WRL::Make<DirectManipulationSystemEventHandler>(window);
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
}