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

        enum DeviceTypeFlags {
            None = 0x0,
            Touchpad = 0x1,
            Pen = 0x2,
            Touch = 0x4,
            Wheel = 0x8,
            All = Touchpad | Pen | Touch | Wheel,
        };
        Q_DECLARE_FLAGS(DeviceType, DeviceTypeFlags)

        static DirectManipulationSystem *instance();
        static void registerWindow(QWindow *window);
        static void registerWindow(QWindow *window, Configuration configuration, DeviceType deviceType);
        static void unregisterWindow(QWindow *window);
        static bool processNativeMessageManually(void *message);

    private:
        QScopedPointer<DirectManipulationSystemPrivate> d_ptr;
    };

}

Q_DECLARE_OPERATORS_FOR_FLAGS(QWDMH::DirectManipulationSystem::Configuration)
Q_DECLARE_OPERATORS_FOR_FLAGS(QWDMH::DirectManipulationSystem::DeviceType)

#endif //QWDMH_DIRECTMANIPULATIONSYSTEM_H
