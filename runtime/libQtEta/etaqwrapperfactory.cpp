#include <cassert>

#include <QAction>
#include <QWidget>
#include <QPushButton>
#include <QCheckBox>
#include <QRadioButton>
#include <QMenu>
#include <QToolBar>
#include <QMenuBar>
#include <QLabel>
#include <QGroupBox>
#include <QMainWindow>
#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QSlider>
#include <QScrollBar>

#include "etaqwrapperfactory.h"
#include "bindqt.h"

template<typename WrapperType, typename QtType>
WrapperType* makeWrapper(QtType* q)
{
    WrapperType* xi = gcNew<WrapperType>();
    xi->setObject(q);
    return xi;
}

template<typename WrapperType>
WrapperType* tryMakeWrapper(QObject* ptr)
{
    typename WrapperType::wrappedType* cptr;
    if (cptr = qobject_cast<typename WrapperType::wrappedType*>(ptr))
        return makeWrapper<WrapperType, typename WrapperType::wrappedType>(cptr);

    return 0;
}

Etaobj* etaqCreateQObjectWrapper(QObject* ptr)
{
    Etaobj* r = 0;
    if (ptr->isWidgetType()) {
        r = r ? r : tryMakeWrapper<EtaQPushButton>(ptr);
        r = r ? r : tryMakeWrapper<EtaQCheckBox>(ptr);
        r = r ? r : tryMakeWrapper<EtaQRadioButton>(ptr);
        r = r ? r : tryMakeWrapper<EtaQMenu>(ptr);
        r = r ? r : tryMakeWrapper<EtaQToolBar>(ptr);
        r = r ? r : tryMakeWrapper<EtaQMenuBar>(ptr);
        r = r ? r : tryMakeWrapper<EtaQLabel>(ptr);
        r = r ? r : tryMakeWrapper<EtaQGroupBox>(ptr);
        r = r ? r : tryMakeWrapper<EtaQMainWindow>(ptr);
        r = r ? r : tryMakeWrapper<EtaQDialog>(ptr);
        r = r ? r : tryMakeWrapper<EtaQLineEdit>(ptr);
        r = r ? r : tryMakeWrapper<EtaQTextEdit>(ptr);
        r = r ? r : tryMakeWrapper<EtaQSlider>(ptr);
        r = r ? r : tryMakeWrapper<EtaQScrollBar>(ptr);
        r = r ? r : tryMakeWrapper<EtaQAbstractSlider>(ptr); // must be below QSlider, QScrollBar
        r = r ? r : makeWrapper<EtaQWidget, QWidget>(static_cast<QWidget*>(ptr));
        return r;
    }

    r = r ? r : tryMakeWrapper<EtaQAction>(ptr);
    r = r ? r : tryMakeWrapper<EtaQButtonGroup>(ptr);
    r = r ? r : tryMakeWrapper<EtaQApplication>(ptr);

    assert(r);
    return r;
}

Etaobj* qobjectToEta(QObject* ptr) {
    if (!ptr)
        return 0;

    Etaobj* o;
    if ((o = etaqGetWrapper(ptr)))
        return o;

    o = etaqCreateQObjectWrapper(ptr);
    assert(etaqGetWrapper(ptr) == o);
    return o;
}

template<typename EtaType, typename QType = typename EtaType::wrappedType>
struct ToEta_Adjustor
{
    EtaType* operator() (QType* orig) {
        return static_cast<EtaType*>(qobjectToEta(static_cast<QObject*>(orig)));
    }
};

EtaQAction* toEta(QAction* act) {
    return ToEta_Adjustor<EtaQAction>()(act);
}

EtaQWidget* toEta(QWidget* w) {
    return ToEta_Adjustor<EtaQWidget>()(w);
}

EtaQPushButton* toEta(QPushButton* b) {
    return ToEta_Adjustor<EtaQPushButton>()(b);
}

EtaQCheckBox* toEta(QCheckBox* b) {
    return ToEta_Adjustor<EtaQCheckBox>()(b);
}

EtaQRadioButton* toEta(QRadioButton* b) {
    return ToEta_Adjustor<EtaQRadioButton>()(b);
}

EtaQButtonGroup* toEta(QButtonGroup* b) {
    return ToEta_Adjustor<EtaQButtonGroup>()(b);
}

EtaQMenu* toEta(QMenu* b) {
    return ToEta_Adjustor<EtaQMenu>()(b);
}

EtaQToolBar* toEta(QToolBar* b) {
    return ToEta_Adjustor<EtaQToolBar>()(b);
}

EtaQApplication* toEta(QApplication* a) {
    return ToEta_Adjustor<EtaQApplication>()(a);
}

EtaQMenuBar* toEta(QMenuBar* b) {
    return ToEta_Adjustor<EtaQMenuBar>()(b);
}

EtaQLabel* toEta(QLabel* b) {
    return ToEta_Adjustor<EtaQLabel>()(b);
}

EtaQGroupBox* toEta(QGroupBox* b) {
    return ToEta_Adjustor<EtaQGroupBox>()(b);
}

EtaQMainWindow* toEta(QMainWindow* b) {
    return ToEta_Adjustor<EtaQMainWindow>()(b);
}

EtaQDialog* toEta(QDialog* b) {
    return ToEta_Adjustor<EtaQDialog>()(b);
}

EtaQLineEdit* toEta(QLineEdit* b) {
    return ToEta_Adjustor<EtaQLineEdit>()(b);
}

EtaQTextEdit* toEta(QTextEdit* b) {
    return ToEta_Adjustor<EtaQTextEdit>()(b);
}

EtaQScrollBar* toEta(QScrollBar* b) {
    return ToEta_Adjustor<EtaQScrollBar>()(b);
}

EtaQSlider* toEta(QSlider* b) {
    return ToEta_Adjustor<EtaQSlider>()(b);
}

