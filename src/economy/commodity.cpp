#include "metternich.h"

#include "economy/commodity.h"

#include "util/assert_util.h"

namespace metternich {

void commodity::check() const
{
	assert_throw(this->get_icon() != nullptr);
}

}
