#include "metternich.h"

#include "infrastructure/dungeon.h"

#include "script/condition/and_condition.h"

namespace metternich {

dungeon::dungeon(const std::string &identifier) : named_data_entry(identifier)
{
}

dungeon::~dungeon()
{
}

void dungeon::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "conditions") {
		auto conditions = std::make_unique<and_condition<site>>();
		conditions->process_gsml_data(scope);
		this->conditions = std::move(conditions);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void dungeon::check() const
{
	if (this->get_portrait() == nullptr) {
		throw std::runtime_error(std::format("Dungeon \"{}\" has no portrait.", this->get_identifier()));
	}

	if (this->get_conditions() != nullptr) {
		this->get_conditions()->check_validity();
	}
}

}
