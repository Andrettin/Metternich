#include "metternich.h"

#include "country/culture.h"

#include "util/assert_util.h"

namespace metternich {

void culture::check() const
{
	assert_throw(this->get_group() != nullptr);

	culture_base::check();
}

}
