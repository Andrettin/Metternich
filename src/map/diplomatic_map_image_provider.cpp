#include "metternich.h"

#include "map/diplomatic_map_image_provider.h"

#include "game/game.h"

namespace metternich {

QImage diplomatic_map_image_provider::requestImage(const QString &id, QSize *size, const QSize &requested_size)
{
	Q_UNUSED(id);
	Q_UNUSED(requested_size);

	const QImage &image = game::get()->get_diplomatic_map_image();

	if (size != nullptr) {
		*size = image.size();
	}

	return image;
}

}