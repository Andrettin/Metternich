#include "metternich.h"

#include "species/creature_size.h"

#include "script/modifier.h"

namespace metternich {

creature_size::creature_size(const std::string &identifier)
	: named_data_entry(identifier)
{
}

creature_size::~creature_size()
{
}

void creature_size::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "modifier") {
		auto modifier = std::make_unique<metternich::modifier<const character>>();
		modifier->process_gsml_data(scope);
		this->modifier = std::move(modifier);
	} else {
		named_data_entry::process_gsml_scope(scope);
	}
}

}
