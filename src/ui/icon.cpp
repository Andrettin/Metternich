#include "metternich.h"

#include "ui/icon.h"

#include "ui/icon_image_provider.h"
#include "util/assert_util.h"
#include "util/event_loop.h"

namespace metternich {

void icon::initialize()
{
	event_loop::get()->co_spawn([this]() -> boost::asio::awaitable<void> {
		co_await icon_image_provider::get()->load_image(this->get_identifier());
	});

	data_entry::initialize();
}

void icon::check() const
{
	assert_throw(!this->get_filepath().empty());
}

void icon::set_filepath(const std::filesystem::path &filepath)
{
	if (filepath == this->get_filepath()) {
		return;
	}

	this->filepath = database::get()->get_graphics_path(this->get_module()) / filepath;
}

}
