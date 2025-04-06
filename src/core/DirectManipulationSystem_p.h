#ifndef QWDMH_DIRECTMANIPULATIONSYSTEM_P_H
#define QWDMH_DIRECTMANIPULATIONSYSTEM_P_H

#include <wrl/module.h>
#include <wrl/client.h>
#include <directmanipulation.h>

#include <QAbstractNativeEventFilter>
#include <QHash>

#include <QWDMHCore/DirectManipulationSystem.h>


namespace QWDMH{
    class DirectManipulationSystemPrivate {
        Q_DECLARE_PUBLIC(DirectManipulationSystem)
    public:
        DirectManipulationSystem *q_ptr;

        struct ViewportContext {
            Microsoft::WRL::ComPtr<IDirectManipulationViewport> viewport;
            Microsoft::WRL::ComPtr<IDirectManipulationUpdateManager> updateManager;
            Microsoft::WRL::ComPtr<IDirectManipulationManager> manager;
            DWORD eventCookie = 0;
            DirectManipulationSystem::DeviceType deviceType;
        };

        class EventFilter : public QAbstractNativeEventFilter {
        public:
            bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;
        } eventFilter;

        QHash<QWindow *, HWND> winIds;
        QHash<HWND, ViewportContext> contexts;

        static DirectManipulationSystemPrivate *instance();
        static bool shouldProcessMessage(MSG *msg);

    };
}

#endif //QWDMH_DIRECTMANIPULATIONSYSTEM_P_H
