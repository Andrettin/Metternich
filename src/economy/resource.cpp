#include "metternich.h"

#include "economy/resource.h"

#include "economy/commodity.h"
#include "util/assert_util.h"

namespace metternich {

void resource::check() const
{
	assert_throw(this->get_commodity() != nullptr);
	assert_throw(this->get_icon() != nullptr);
}

const metternich::icon *resource::get_icon() const
{
	if (this->icon != nullptr) {
		return this->icon;
	}

	return this->get_commodity()->get_icon();
}

}
