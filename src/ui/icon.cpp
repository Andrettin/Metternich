#include "metternich.h"

#include "ui/icon.h"

#include "ui/icon_image_provider.h"

namespace metternich {

void icon::initialize()
{
	icon_image_provider::get()->load_image(this->get_identifier());

	data_entry::initialize();
}

}
