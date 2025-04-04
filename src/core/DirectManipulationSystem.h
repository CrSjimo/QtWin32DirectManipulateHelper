#ifndef QWDMH_DIRECTMANIPULATIONSYSTEM_H
#define QWDMH_DIRECTMANIPULATIONSYSTEM_H

#include <QObject>

#include <QWDMHCore/QWDMHCoreGlobal.h>

class QWindow;

namespace QWDMH {

    class DirectManipulationSystemPrivate;

    class QWDMH_CORE_EXPORT DirectManipulationSystem final : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(DirectManipulationSystem)
    public:
        explicit DirectManipulationSystem(QObject *parent = nullptr);
        ~DirectManipulationSystem() override;

        static DirectManipulationSystem *instance();
        static void registerWindow(QWindow *window);
        static void unregisterWindow(QWindow *window);

    private:
        QScopedPointer<DirectManipulationSystemPrivate> d_ptr;
    };

}

#endif //QWDMH_DIRECTMANIPULATIONSYSTEM_H
