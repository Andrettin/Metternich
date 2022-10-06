#include "metternich.h"

#include "map/province.h"

#include "util/assert_util.h"

namespace metternich {

void province::check() const
{
	assert_throw(this->get_color().isValid());
}

}
