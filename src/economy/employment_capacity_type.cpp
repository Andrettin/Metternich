#include "metternich.h"

#include "economy/employment_capacity_type.h"

namespace metternich {
	
employment_capacity_type::employment_capacity_type(const std::string &identifier) : named_data_entry(identifier)
{
}

employment_capacity_type::~employment_capacity_type()
{
}

void employment_capacity_type::check() const
{
	if (this->get_employment_types().empty()) {
		throw std::runtime_error(std::format("Employment capacity type \"{}\" has no employment types.", this->get_identifier()));
	}

	named_data_entry::check();
}

}

#include "moc_employment_capacity_type.cpp"
