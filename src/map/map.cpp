#include "metternich.h"

#include "map/map.h"

#include "util/point_util.h"

namespace metternich {

int map::get_pos_index(const QPoint &pos) const
{
	return point::to_index(pos, this->get_width());
}

}
