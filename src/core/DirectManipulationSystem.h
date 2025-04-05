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

        enum ConfigurationFlags {
            TranslationX = 0x2,
            TranslationY = 0x4,
            Scaling = 0x10,
            TranslationInertia = 0x20,
            ScalingInertia = 0x80,
            RailsX = 0x100,
            RailsY = 0x200,
        };
        Q_DECLARE_FLAGS(Configuration, ConfigurationFlags)

        static DirectManipulationSystem *instance();
        static void registerWindow(QWindow *window);
        static void registerWindow(QWindow *window, Configuration configuration);
        static void unregisterWindow(QWindow *window);

    private:
        QScopedPointer<DirectManipulationSystemPrivate> d_ptr;
    };

}

Q_DECLARE_OPERATORS_FOR_FLAGS(QWDMH::DirectManipulationSystem::Configuration)

#endif //QWDMH_DIRECTMANIPULATIONSYSTEM_H
