#include "metternich.h"

#include "ui/icon.h"

#include "script/condition/and_condition.h"
#include "ui/icon_image_provider.h"

namespace metternich {

icon::icon(const std::string &identifier) : icon_base(identifier)
{
}

icon::~icon()
{
}

void icon::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "character_conditions") {
		auto conditions = std::make_unique<and_condition<character>>();
		conditions->process_gsml_data(scope);
		this->character_conditions = std::move(conditions);
	} else {
		icon_base::process_gsml_scope(scope);
	}
}

void icon::initialize()
{
	QTimer::singleShot(0, [this]() -> QCoro::Task<void> {
		co_await icon_image_provider::get()->load_image(this->get_identifier());
	});

	if (this->is_character_icon()) {
		icon::character_icons.push_back(this);
	}

	data_entry::initialize();
}

}
