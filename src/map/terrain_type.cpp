#include "metternich.h"

#include "map/terrain_type.h"

#include "util/assert_util.h"

namespace metternich {

void terrain_type::check() const
{
	assert_throw(this->get_color().isValid());
}

}
