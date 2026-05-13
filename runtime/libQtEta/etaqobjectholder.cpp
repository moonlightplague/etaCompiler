#include "etaqobjectholder.h"

#include <QHash>

static QHash<QObject*, Etaobj*>* s_wrappers;

static QHash<QObject*, Etaobj*>& wrappers() {
    if (!s_wrappers)
        s_wrappers = new QHash<QObject*, Etaobj*>;
    return *s_wrappers;
}

Etaobj* etaqGetWrapper(QObject* object)
{
    return wrappers().value(object);
}

void etaqPutWrapper(QObject* object, Etaobj* wrapper)
{
    assert (!wrappers().contains(object));
    wrappers().insert(object, wrapper);
}

void etaqForgetWrapper(QObject* object)
{
    assert (wrappers().contains(object));
    wrappers().remove(object);
}

// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
