#pragma once

#pragma warning(push, 0)
#include <QQuickImageProvider>
#pragma warning(pop)

namespace metternich {

class diplomatic_map_image_provider final : public QQuickImageProvider
{
public:
	diplomatic_map_image_provider() : QQuickImageProvider(QQuickImageProvider::Image)
	{
	}

	virtual QImage requestImage(const QString &id, QSize *size, const QSize &requested_size) override;
};

}
