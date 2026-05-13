#include <etaqslot.h>
#include <bindqt.h>

EtaQPushButtonSigs::EtaQPushButtonSigs(EtaQPushButton* etaObj, QPushButton* qObj):
    etaObj(etaObj), qObj(qObj)
{
    connect(qObj, SIGNAL(clicked(bool)),
            this, SLOT(slotClicked(bool)));
}

void EtaQPushButtonSigs::slotClicked(bool)
{
    // Dispatch to the peer, and all its listeners.
    etaObj->vtable()->clicked(etaObj);

    etaqEventListenerList<EtaClickListener>::Cell* cur;
    for (cur = etaObj->clickListeners.head; cur; cur = cur->next)
        cur->listener->vtable()->clicked(cur->listener, etaObj);
}

///////////////////////////////////////////////////////////////////////////////
EtaQTimerSigs::EtaQTimerSigs(EtaQTimer* etaObj, QTimer* qObj):
    etaObj(etaObj), qObj(qObj)
{
    connect(qObj, SIGNAL(timeout()),
            this, SLOT(slotTimeout()));
}

void EtaQTimerSigs::slotTimeout()
{
    // Dispatch to the peer, and all its listeners.
    etaObj->vtable()->timeout(etaObj);

    etaqEventListenerList<EtaTimerListener>::Cell* cur;
    for (cur = etaObj->timerListeners.head; cur; cur = cur->next)
        cur->listener->vtable()->timeout(cur->listener, etaObj);
}


///////////////////////////////////////////////////////////////////////////////
EtaQApplicationSigs::EtaQApplicationSigs(EtaQApplication* etaObj, QApplication* qObj):
    etaObj(etaObj), qObj(qObj)
{
    connect(qObj, SIGNAL(aboutToQuit()),
            this, SLOT(slotAboutToQuit()));
}

void EtaQApplicationSigs::slotAboutToQuit()
{
    // Dispatch to the peer, and all its listeners.
    etaqEventListenerList<EtaAboutToQuitListener>::Cell* cur;
    for (cur = etaObj->quitListeners.head; cur; cur = cur->next)
        cur->listener->vtable()->aboutToQuit(cur->listener);
}

///////////////////////////////////////////////////////////////////////////////
EtaQActionSigs::EtaQActionSigs(EtaQAction* etaObj, QAction* qObj):
    etaObj(etaObj), qObj(qObj)
{
    connect(qObj, SIGNAL(triggered(bool)),
            this, SLOT(slotTriggered()));
}

void EtaQActionSigs::slotTriggered()
{
    etaObj->vtable()->triggered(etaObj);
    etaqEventListenerList<EtaActionListener>::Cell* cur;
    for (cur = etaObj->actionListeners.head; cur; cur = cur->next)
        cur->listener->vtable()->triggered(cur->listener, etaObj);
}

///////////////////////////////////////////////////////////////////////////////
EtaQCheckBoxSigs::EtaQCheckBoxSigs(EtaQCheckBox* etaObj, QCheckBox* qObj):
    etaObj(etaObj), qObj(qObj)
{
    connect(qObj, SIGNAL(toggled(bool)),
            this, SLOT(slotToggled(bool)));
}

void EtaQCheckBoxSigs::slotToggled(bool v)
{
    etaObj->vtable()->toggled(etaObj, v);
    etaqEventListenerList<EtaToggleListener>::Cell* cur;
    for (cur = etaObj->listeners.head; cur; cur = cur->next)
        cur->listener->vtable()->toggled(cur->listener, etaObj, v);
}

///////////////////////////////////////////////////////////////////////////////
EtaQRadioButtonSigs::EtaQRadioButtonSigs(EtaQRadioButton* etaObj, QRadioButton* qObj):
    etaObj(etaObj), qObj(qObj)
{
    connect(qObj, SIGNAL(toggled(bool)),
            this, SLOT(slotToggled(bool)));
}

void EtaQRadioButtonSigs::slotToggled(bool v)
{
    etaObj->vtable()->toggled(etaObj, v);
    etaqEventListenerList<EtaToggleListener>::Cell* cur;
    for (cur = etaObj->listeners.head; cur; cur = cur->next)
        cur->listener->vtable()->toggled(cur->listener, etaObj, v);
}

///////////////////////////////////////////////////////////////////////////////
EtaQAbstractSliderSigs::EtaQAbstractSliderSigs(EtaQAbstractSlider* etaObj, QAbstractSlider* qObj):
    etaObj(etaObj), qObj(qObj)
{
    connect(qObj, SIGNAL(sliderMoved(int)),
            this, SLOT(slotSliderMoved(int)));
}

void EtaQAbstractSliderSigs::slotSliderMoved(int v)
{
    etaObj->vtable()->sliderMoved(etaObj, v);
    etaqEventListenerList<EtaSliderListener>::Cell* cur;
    for (cur = etaObj->sliderListeners.head; cur; cur = cur->next)
        cur->listener->vtable()->sliderMoved(cur->listener, etaObj, v);
}

