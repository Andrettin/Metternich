#include "metternich.h"

#include "character/saving_throw_type.h"

namespace metternich {

void saving_throw_type::initialize()
{
	if (this->base_saving_throw_type != nullptr) {
		this->base_saving_throw_type->derived_saving_throw_types.push_back(this);
	}

	named_data_entry::initialize();
}

void saving_throw_type::check() const
{
}

}
