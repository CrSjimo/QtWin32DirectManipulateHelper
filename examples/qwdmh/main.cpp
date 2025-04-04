#include <QApplication>
#include <QPainter>
#include <QCheckBox>
#include <QtEvents>

#include <QWDMHCore/DirectManipulationSystem.h>

using namespace QWDMH;

class DirectWidget : public QWidget {
    Q_OBJECT
public:
    explicit DirectWidget(QWidget* parent = nullptr) : QWidget(parent) {
        auto checkBox = new QCheckBox("Direct Manipulate");
        checkBox->setParent(this);
        connect(checkBox, &QCheckBox::clicked, this, [=](bool checked) {
            if (checked) {
                DirectManipulationSystem::registerWindow(this->windowHandle());
            } else {
                DirectManipulationSystem::unregisterWindow(this->windowHandle());
            }
        });
    }

protected:

    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.fillRect(rect(), Qt::blue);

        painter.save();
        painter.translate(m_tx, m_ty);
        painter.scale(m_scale, m_scale);
        painter.fillRect(100, 100, 200, 150, Qt::green);
        painter.restore();

        painter.drawText(10, 30, QString("Scale: %1\nX: %2\nY: %3").arg(m_scale).arg(m_tx).arg(m_ty));
    }

    bool event(QEvent* e) override {
        if (e->type() == QEvent::NativeGesture) {
            auto event = static_cast<QNativeGestureEvent *>(e);
            if (event->gestureType() == Qt::ZoomNativeGesture) {
                m_scale *= event->value() + 1;
                update();
            }
        }
        return QWidget::event(e);
    }

    void wheelEvent(QWheelEvent* event) override {
        m_tx += event->angleDelta().x() * 0.5;
        m_ty += event->angleDelta().y() * 0.5;
        update();
    }

private:
    float m_scale = 1.0f;
    float m_tx = 0.0f;
    float m_ty = 0.0f;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    DirectManipulationSystem dms;
    DirectWidget widget;
    widget.resize(800, 600);
    widget.show();
    return app.exec();
}

#include "main.moc"