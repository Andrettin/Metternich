#include "metternich.h"

#include "item/object_type.h"

namespace metternich {

object_type::object_type(const std::string &identifier)
	: named_data_entry(identifier)
{
}

object_type::~object_type()
{
}

void object_type::check() const
{
	if (this->get_icon() == nullptr) {
		throw std::runtime_error(std::format("Object type \"{}\" has no icon.", this->get_identifier()));
	}
}

}
