#ifndef EtaQ_SLOT_H
#define EtaQ_SLOT_H

#include <etaobj.h>

#include <QObject>
#include <QCheckBox>
#include <QPushButton>
#include <QRadioButton>
#include <QAbstractSlider>

// An ultra-simple list that's stored in the GC heap,
// so it can be traced accross (as the normal heap can't be)
template<typename T>
struct etaqEventListenerList
{
    struct Cell
    {
        T* listener;
        Cell* next;
    };

    void append(T* listener) {
        Cell* newCell = gcCalloc<Cell>();
        newCell->listener = listener;

        if (head) {
            tail->next = newCell;
            tail = newCell;
        } else {
            head = tail = newCell;
        }
    }

    Cell* head;
    Cell* tail;

    etaqEventListenerList(): head(0), tail(0)
    {}
};

struct EtaQPushButton;
class EtaQPushButtonSigs: public QObject
{
    Q_OBJECT
public:
    EtaQPushButtonSigs(EtaQPushButton* etaObj, QPushButton* qObj);
private slots:
    void slotClicked(bool);
private:
    EtaQPushButton* etaObj;
    QPushButton*   qObj;
};

struct EtaQTimer;
class EtaQTimerSigs: public QObject
{
    Q_OBJECT
public:
    EtaQTimerSigs(EtaQTimer* etaObj, QTimer* qObj);
private slots:
    void slotTimeout();
private:
    EtaQTimer* etaObj;
    QTimer*   qObj;
};


struct EtaQApplication;
class EtaQApplicationSigs: public QObject
{
    Q_OBJECT
public:
    EtaQApplicationSigs(EtaQApplication* etaObj, QApplication* qObj);
private slots:
    void slotAboutToQuit();
private:
    EtaQApplication* etaObj;
    QApplication*   qObj;
};

struct EtaQAction;
class EtaQActionSigs: public QObject
{
    Q_OBJECT
public:
    EtaQActionSigs(EtaQAction* etaObj, QAction* qObj);
private slots:
    void slotTriggered();
private:
    EtaQAction* etaObj;
    QAction*   qObj;
};

struct EtaQCheckBox;
class EtaQCheckBoxSigs: public QObject
{
    Q_OBJECT
public:
    EtaQCheckBoxSigs(EtaQCheckBox* etaObj, QCheckBox* qObj);
private slots:
    void slotToggled(bool);
private:
    EtaQCheckBox* etaObj;
    QCheckBox*   qObj;
};

struct EtaQRadioButton;
class EtaQRadioButtonSigs: public QObject
{
    Q_OBJECT
public:
    EtaQRadioButtonSigs(EtaQRadioButton* etaObj, QRadioButton* qObj);
private slots:
    void slotToggled(bool);
private:
    EtaQRadioButton* etaObj;
    QRadioButton*   qObj;
};

struct EtaQAbstractSlider;
class EtaQAbstractSliderSigs: public QObject
{
    Q_OBJECT
public:
    EtaQAbstractSliderSigs(EtaQAbstractSlider* etaObj, QAbstractSlider* qObj);
private slots:
    void slotSliderMoved(int);
private:
    EtaQAbstractSlider* etaObj;
    QAbstractSlider*   qObj;
};


#endif
