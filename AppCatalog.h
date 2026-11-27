#pragma once

#include <QQuickImageProvider>

// Serves icons by theme name or absolute path: image://aethericon/<name>
class IconProvider : public QQuickImageProvider
{
public:
    IconProvider() : QQuickImageProvider(QQuickImageProvider::Pixmap) {}
    QPixmap requestPixmap(const QString &id, QSize *size,
                          const QSize &requestedSize) override;
};
