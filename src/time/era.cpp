#include "metternich.h"

#include "time/era.h"

#include "util/assert_util.h"
#include "util/vector_util.h"

namespace metternich {
	
void era::initialize_all()
{
	data_type::initialize_all();

	era::sort_instances([](const era *lhs, const era *rhs) {
		if (lhs->get_start_date() != rhs->get_start_date()) {
			return lhs->get_start_date() < rhs->get_start_date();
		} else {
			return lhs->get_identifier() < rhs->get_identifier();
		}
	});
}

void era::check() const
{
	assert_throw(this->get_start_date().isValid());
}

}
