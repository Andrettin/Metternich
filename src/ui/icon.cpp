#include "metternich.h"

#include "ui/icon.h"

#include "ui/icon_image_provider.h"
#include "util/event_loop.h"

namespace metternich {

void icon::initialize()
{
	QTimer::singleShot(0, this, [this]() -> QCoro::Task<void> {
		co_await icon_image_provider::get()->load_image(this->get_identifier());
	});

	data_entry::initialize();
}

}
