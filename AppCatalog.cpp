#include "AppCatalog.h"

#include <QIcon>
#include <QPixmap>

QPixmap IconProvider::requestPixmap(const QString &id, QSize *size,
                                    const QSize &requestedSize)
{
    const int pixelSize = requestedSize.isValid() && requestedSize.width() > 0
                              ? requestedSize.width() : 128;
    const QIcon icon = id.startsWith('/') ? QIcon(id) : QIcon::fromTheme(id);
    if (icon.isNull())
        return {};

    const QPixmap pixmap = icon.pixmap(QSize(pixelSize, pixelSize));
    if (size)
        *size = pixmap.size();
    return pixmap;
}
