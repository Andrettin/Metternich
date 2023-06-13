#include "metternich.h"

#include "unit/transporter_class.h"

#include "unit/transporter_category.h"
#include "util/assert_util.h"
#include "util/vector_util.h"

namespace metternich {

transporter_class::transporter_class(const std::string &identifier)
	: named_data_entry(identifier), category(transporter_category::none)
{
}

void transporter_class::check() const
{
	assert_throw(this->get_category() != transporter_category::none);
}

void transporter_class::set_default_transporter_type(const transporter_type *transporter_type)
{
	assert_throw(this->get_default_transporter_type() == nullptr);

	this->default_transporter_type = transporter_type;
}

}
