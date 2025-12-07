#include "metternich.h"

#include "ui/portrait.h"

#include "script/condition/and_condition.h"
#include "ui/portrait_image_provider.h"

namespace metternich {

portrait::portrait(const std::string &identifier) : icon_base(identifier)
{
}

portrait::~portrait()
{
}

void portrait::process_gsml_scope(const gsml_data &scope)
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

void portrait::initialize()
{
	QTimer::singleShot(0, [this]() -> QCoro::Task<void> {
		co_await portrait_image_provider::get()->load_image(this->get_identifier());
	});

	if (this->is_character_portrait()) {
		portrait::character_portraits.push_back(this);
	}

	data_entry::initialize();
}

void portrait::check() const
{
	if (this->get_character_conditions() != nullptr) {
		this->get_character_conditions()->check_validity();
	}

	icon_base::check();
}

}
