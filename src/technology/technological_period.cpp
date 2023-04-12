#include "metternich.h"

#include "technology/technological_period.h"

#include "util/assert_util.h"
#include "util/vector_util.h"

namespace metternich {
	
void technological_period::initialize_all()
{
	data_type::initialize_all();

	technological_period::sort_instances([](const technological_period *lhs, const technological_period *rhs) {
		assert_throw(lhs->get_start_year() != rhs->get_start_year());
		return lhs->get_start_year() < rhs->get_start_year();
	});

	for (size_t i = 0; i < technological_period::get_all().size(); ++i) {
		technological_period *period = technological_period::get_all().at(i);

		period->index = static_cast<int>(i);

		if (i > 0) {
			const technological_period *previous_period = technological_period::get_all().at(i - 1);
			assert_throw(period->get_start_year() == previous_period->get_end_year());
		}
	}
}

void technological_period::initialize()
{
	assert_throw(!technological_period::periods_by_year.contains(this->get_start_year()));
	technological_period::periods_by_year[this->get_start_year()] = this;

	data_entry::initialize();
}

void technological_period::check() const
{
	assert_throw(this->get_start_year() != 0);
	assert_throw(this->get_end_year() != 0);
	assert_throw(this->get_end_year() > this->get_start_year());
}

}
