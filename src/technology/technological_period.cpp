#include "metternich.h"

#include "technology/technological_period.h"

#include "util/assert_util.h"
#include "util/vector_util.h"

namespace metternich {
	
void technological_period::initialize()
{
	if (this->parent_period != nullptr) {
		assert_throw(this->parent_period != this);

		if (!this->parent_period->is_initialized()) {
			this->parent_period->initialize();
		}

		this->parent_period->child_periods.push_back(this);

		this->index = this->parent_period->get_index() + 1;
	} else {
		this->index = 0;
	}

	data_entry::initialize();
}

void technological_period::check() const
{
	if (this->parent_period != nullptr) {
		assert_throw(this->get_start_year() == this->parent_period->get_end_year());
	}

	assert_throw(this->get_index() != -1);
	assert_throw(this->get_start_year() != 0);
	assert_throw(this->get_end_year() != 0);
	assert_throw(this->get_end_year() > this->get_start_year());
}

}
