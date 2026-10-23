#include "metternich.h"

#include "character/save_type.h"

namespace metternich {

void save_type::initialize()
{
	if (this->base_save_type != nullptr) {
		this->base_save_type->add_derived_stat(this);
	}

	character_stat::initialize();
}

void save_type::check() const
{
	character_stat::check();
}

}

#include "moc_save_type.cpp"
