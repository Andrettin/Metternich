#include "metternich.h"

#include "population/population_unit.h"

#include "map/province.h"
#include "map/province_game_data.h"
#include "population/population_type.h"
#include "ui/icon.h"
#include "util/assert_util.h"

namespace metternich {

population_unit::population_unit(const population_type *type, const metternich::province *province)
	: type(type), province(province)
{
	assert_throw(this->get_type() != nullptr);
	assert_throw(this->get_province() != nullptr);

	connect(this, &population_unit::type_changed, this, &population_unit::icon_changed);
}

const icon *population_unit::get_icon() const
{
	return this->get_type()->get_icon();
}

void population_unit::set_province(const metternich::province *province)
{
	if (province == this->get_province()) {
		return;
	}

	this->province = province;

	emit province_changed();
}

}
