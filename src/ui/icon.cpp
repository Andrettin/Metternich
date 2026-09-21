#include "metternich.h"

#include "ui/icon.h"

#include "ui/icon_image_provider.h"

namespace metternich {

icon::icon(const std::string &identifier) : icon_base(identifier)
{
}

icon::~icon()
{
}

void icon::initialize()
{
	QTimer::singleShot(0, [this]() -> QCoro::Task<void> {
		co_await icon_image_provider::get()->load_image(this->get_identifier());
	});

	data_entry::initialize();
}

}
