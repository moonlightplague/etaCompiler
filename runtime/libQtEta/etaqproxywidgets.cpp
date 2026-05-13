#include <etaqproxywidgets.h>
#include <etaqobjectholder.h> // for getWrapper
#include <bindqt.h>

template<typename EtaType, typename QType = typename EtaType::wrappedType>
struct ToEta_Event
{
    EtaType* operator() (QType* orig) {
        EtaType* o = gcNew<EtaType>();
        o->e = orig;
        return o;
    }
};

template<typename T>
void Proxy<T>::paintEvent(QPaintEvent* pe)
{
    // Find our wrapper. It must exist, since we were created by one.
    EtaQWidget* wrapper = static_cast<EtaQWidget*>(etaqGetWrapper(this));
    wrapper->vtable()->paintEvent(wrapper, ToEta_Event<EtaQPaintEvent>()(pe));
}

template<typename T>
void Proxy<T>::mousePressEvent(QMouseEvent* e)
{
    EtaQWidget* wrapper = static_cast<EtaQWidget*>(etaqGetWrapper(this));
    wrapper->vtable()->mousePressEvent(wrapper, ToEta_Event<EtaQMouseEvent>()(e));
}

template<typename T>
void Proxy<T>::mouseReleaseEvent(QMouseEvent* e)
{
    EtaQWidget* wrapper = static_cast<EtaQWidget*>(etaqGetWrapper(this));
    wrapper->vtable()->mouseReleaseEvent(wrapper, ToEta_Event<EtaQMouseEvent>()(e));
}

template<typename T>
void Proxy<T>::mouseDoubleClickEvent(QMouseEvent* e)
{
    EtaQWidget* wrapper = static_cast<EtaQWidget*>(etaqGetWrapper(this));
    wrapper->vtable()->mouseDoubleClickEvent(wrapper, ToEta_Event<EtaQMouseEvent>()(e));
}

template<typename T>
void Proxy<T>::mouseMoveEvent(QMouseEvent* e)
{
    EtaQWidget* wrapper = static_cast<EtaQWidget*>(etaqGetWrapper(this));
    wrapper->vtable()->mouseMoveEvent(wrapper, ToEta_Event<EtaQMouseEvent>()(e));
}

template<typename T>
void Proxy<T>::wheelEvent(QWheelEvent* e)
{
    EtaQWidget* wrapper = static_cast<EtaQWidget*>(etaqGetWrapper(this));
    wrapper->vtable()->wheelEvent(wrapper, ToEta_Event<EtaQWheelEvent>()(e));
}

template<typename T>
void Proxy<T>::keyPressEvent(QKeyEvent* e)
{
    EtaQWidget* wrapper = static_cast<EtaQWidget*>(etaqGetWrapper(this));
    wrapper->vtable()->keyPressEvent(wrapper, ToEta_Event<EtaQKeyEvent>()(e));
}

template<typename T>
void Proxy<T>::keyReleaseEvent(QKeyEvent* e)
{
    EtaQWidget* wrapper = static_cast<EtaQWidget*>(etaqGetWrapper(this));
    wrapper->vtable()->keyReleaseEvent(wrapper, ToEta_Event<EtaQKeyEvent>()(e));
}

template<typename T>
void Proxy<T>::enterEvent(QEvent* e)
{
    EtaQWidget* wrapper = static_cast<EtaQWidget*>(etaqGetWrapper(this));
    wrapper->vtable()->enterEvent(wrapper, ToEta_Event<EtaQEvent>()(e));
}

template<typename T>
void Proxy<T>::leaveEvent(QEvent* e)
{
    EtaQWidget* wrapper = static_cast<EtaQWidget*>(etaqGetWrapper(this));
    wrapper->vtable()->leaveEvent(wrapper, ToEta_Event<EtaQEvent>()(e));
}

template<typename T>
void Proxy<T>::resizeEvent(QResizeEvent* e)
{
    EtaQWidget* wrapper = static_cast<EtaQWidget*>(etaqGetWrapper(this));
    wrapper->vtable()->resizeEvent(wrapper, ToEta_Event<EtaQResizeEvent>()(e));
}

template<typename T>
void Proxy<T>::closeEvent(QCloseEvent* e)
{
    EtaQWidget* wrapper = static_cast<EtaQWidget*>(etaqGetWrapper(this));
    wrapper->vtable()->closeEvent(wrapper, ToEta_Event<EtaQEvent>()(e));
}

template<typename T>
void Proxy<T>::contextMenuEvent(QContextMenuEvent* e)
{
    EtaQWidget* wrapper = static_cast<EtaQWidget*>(etaqGetWrapper(this));
    wrapper->vtable()->contextMenuEvent(wrapper, ToEta_Event<EtaQContextMenuEvent>()(e));
}

template<typename T>
void Proxy<T>::showEvent(QShowEvent* e)
{
    EtaQWidget* wrapper = static_cast<EtaQWidget*>(etaqGetWrapper(this));
    wrapper->vtable()->showEvent(wrapper, ToEta_Event<EtaQEvent>()(e));
}

template<typename T>
void Proxy<T>::hideEvent(QHideEvent* e)
{
    EtaQWidget* wrapper = static_cast<EtaQWidget*>(etaqGetWrapper(this));
    wrapper->vtable()->hideEvent(wrapper, ToEta_Event<EtaQEvent>()(e));
}

template<typename T>
void Proxy<T>::defaultEvent(QEvent* e)
{
    // Matches the code in QWidget::event, for the types we support.
    switch (e->type()) {
    case QEvent::MouseMove:
        T::mouseMoveEvent(static_cast<QMouseEvent*>(e));
        break;
    case QEvent::MouseButtonPress:
        T::mousePressEvent(static_cast<QMouseEvent*>(e));
        break;
    case QEvent::MouseButtonRelease:
        T::mouseReleaseEvent(static_cast<QMouseEvent*>(e));
        break;
    case QEvent::MouseButtonDblClick:
        T::mouseDoubleClickEvent(static_cast<QMouseEvent*>(e));
        break;
    case QEvent::ContextMenu:
        T::contextMenuEvent(static_cast<QContextMenuEvent*>(e));
        break;
    case QEvent::Wheel:
        T::wheelEvent(static_cast<QWheelEvent*>(e));
        break;
    case QEvent::KeyPress:
        T::keyPressEvent(static_cast<QKeyEvent*>(e));
        break;
    case QEvent::KeyRelease:
        T::keyReleaseEvent(static_cast<QKeyEvent*>(e));
        break;
    case QEvent::Enter:
        T::enterEvent(e);
        break;
    case QEvent::Leave:
        T::leaveEvent(e);
        break;
    case QEvent::Resize:
        T::resizeEvent(static_cast<QResizeEvent*>(e));
        break;
    case QEvent::Close:
        T::closeEvent(static_cast<QCloseEvent*>(e));
        break;
    case QEvent::Show:
        T::showEvent(static_cast<QShowEvent*>(e));
        break;
    case QEvent::Hide:
        T::hideEvent(static_cast<QHideEvent*>(e));
        break;
    case QEvent::Paint:
        T::paintEvent(static_cast<QPaintEvent*>(e));
        break;
    default:
        assert(0);
    }
}

// Force instanciation.
template class Proxy<QWidget>;
template class Proxy<QPushButton>;
template class Proxy<QCheckBox>;
template class Proxy<QRadioButton>;
template class Proxy<QMenu>;
template class Proxy<QToolBar>;
template class Proxy<QMenuBar>;
template class Proxy<QLabel>;
template class Proxy<QGroupBox>;
template class Proxy<QMainWindow>;
template class Proxy<QLineEdit>;
template class Proxy<QDialog>;
template class Proxy<QTextEdit>;
template class Proxy<QAbstractSlider>;
template class Proxy<QSlider>;
template class Proxy<QScrollBar>;
