#include "metternich.h"

#include "unit/transporter_class.h"

#include "language/name_generator.h"
#include "unit/transporter_category.h"
#include "unit/transporter_type.h"
#include "util/assert_util.h"
#include "util/vector_util.h"

namespace metternich {

void transporter_class::propagate_names(const transporter_class_map<std::unique_ptr<name_generator>> &name_generators, std::unique_ptr<name_generator> &ship_name_generator)
{
	for (const auto &kv_pair : name_generators) {
		const transporter_class *transporter_class = kv_pair.first;

		if (transporter_class->is_ship()) {
			if (ship_name_generator == nullptr) {
				ship_name_generator = std::make_unique<name_generator>();
			}

			ship_name_generator->add_names(kv_pair.second->get_names());
		}
	}
}

transporter_class::transporter_class(const std::string &identifier)
	: named_data_entry(identifier), category(transporter_category::none)
{
}

void transporter_class::check() const
{
	assert_throw(this->get_category() != transporter_category::none);
}

bool transporter_class::is_ship() const
{
	switch (this->get_category()) {
		case transporter_category::small_merchant_ship:
		case transporter_category::large_merchant_ship:
			return true;
		default:
			return false;
	}
}

void transporter_class::set_default_transporter_type(const transporter_type *transporter_type)
{
	if (this->get_default_transporter_type() != nullptr) {
		throw std::runtime_error(std::format("Cannot set \"{}\" as the default transporter type of class \"{}\", as it already has \"{}\" as its default type.", transporter_type->get_identifier(), this->get_identifier(), this->get_default_transporter_type()->get_identifier()));
	}

	this->default_transporter_type = transporter_type;
}

}
