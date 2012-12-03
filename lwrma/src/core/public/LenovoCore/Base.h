#ifndef __Base_h__
#define __Base_h__

#include <QtCore/QObject>
#include <QtCore/QMetaType>

#ifdef LENOVOCORE_STATICLIB
#define LENOVOCORE_API
#else
#ifdef BUILD_LENOVOCORE
#define LENOVOCORE_API Q_DECL_EXPORT
#else
#define LENOVOCORE_API Q_DECL_IMPORT
#endif
#endif

#endif // __Base_h__
