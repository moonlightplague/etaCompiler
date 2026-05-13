#ifndef EtaQ_WRAPPER_FACTORY_H
#define EtaQ_WRAPPER_FACTORY_H

#include <QObject>
#include <etaobj.h>
#include <etaqobjectholder.h>

struct EtaQAction;
struct EtaQWidget;
struct EtaQPushButton;
struct EtaQApplication;
struct EtaQCheckBox;
struct EtaQRadioButton;
struct EtaQButtonGroup;
struct EtaQMenu;
struct EtaQToolBar;
struct EtaQMenuBar;
struct EtaQLabel;
struct EtaQGroupBox;
struct EtaQMainWindow;
struct EtaQDialog;
struct EtaQLineEdit;
struct EtaQTextEdit;
struct EtaQSlider;
struct EtaQScrollBar;

// This method creates the wrapper of an appropriate type;
// e.g. if given a pure QWidget it will return a EtaQWidget,
// while if it's given a QPushButton, it will return a EtaQPushButton, etc.
Etaobj* etaqCreateQObjectWrapper(QObject* ptr);

EtaQAction*      toEta(QAction* act);
EtaQPushButton*  toEta(QPushButton* b);
EtaQCheckBox*    toEta(QCheckBox* b);
EtaQRadioButton* toEta(QRadioButton* b);
EtaQWidget*      toEta(QWidget* w);
EtaQApplication* toEta(QApplication* a);
EtaQButtonGroup* toEta(QButtonGroup* a);
EtaQMenu*        toEta(QMenu* m);
EtaQToolBar*     toEta(QToolBar* t);
EtaQMenuBar*     toEta(QMenuBar* m);
EtaQLabel*       toEta(QLabel* m);
EtaQMainWindow*  toEta(QMainWindow* m);
EtaQGroupBox*    toEta(QGroupBox* m);
EtaQDialog*      toEta(QDialog* m);
EtaQLineEdit*    toEta(QLineEdit* m);
EtaQTextEdit*    toEta(QTextEdit* m);
EtaQScrollBar*   toEta(QScrollBar* m);
EtaQSlider*      toEta(QSlider* m);
#endif
