#include "metternich.h"

#include "ui/portrait.h"

#include "ui/portrait_image_provider.h"
#include "util/event_loop.h"

namespace metternich {

void portrait::initialize()
{
	event_loop::get()->co_spawn([this]() -> boost::asio::awaitable<void> {
		co_await portrait_image_provider::get()->load_image(this->get_identifier());
	});

	data_entry::initialize();
}

}
