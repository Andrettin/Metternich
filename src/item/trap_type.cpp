#include "metternich.h"

#include "item/trap_type.h"

#include "script/effect/effect_list.h"

namespace metternich {

trap_type::trap_type(const std::string &identifier)
	: named_data_entry(identifier)
{
}

trap_type::~trap_type()
{
}

void trap_type::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "on_triggered") {
		this->trigger_effects = std::make_unique<effect_list<const character>>();
		this->trigger_effects->process_gsml_data(scope);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void trap_type::check() const
{
	if (this->get_trigger_effects() == nullptr) {
		throw std::runtime_error(std::format("Trap type \"{}\" has trigger effects.", this->get_identifier()));
	}
}

}
