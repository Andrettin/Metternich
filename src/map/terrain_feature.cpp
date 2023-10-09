#include "metternich.h"

#include "map/terrain_feature.h"

#include "util/assert_util.h"

namespace metternich {

void terrain_feature::check() const
{
	assert_throw(this->get_terrain_type() != nullptr || this->is_river() || this->is_border_river());
	assert_throw(!(this->get_terrain_type() != nullptr && (this->is_river() || this->is_border_river())));
}

}
