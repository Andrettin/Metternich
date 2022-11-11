#include "metternich.h"

#include "population/population_unit.h"

#include "country/culture.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "population/population_type.h"
#include "ui/icon.h"
#include "util/assert_util.h"

namespace metternich {

population_unit::population_unit(const population_type *type, const metternich::culture *culture, const metternich::phenotype *phenotype, const metternich::province *province)
	: type(type), culture(culture), phenotype(phenotype), province(province)
{
	assert_throw(this->get_type() != nullptr);
	assert_throw(this->get_culture() != nullptr);
	assert_throw(this->get_phenotype() != nullptr);
	assert_throw(this->get_province() != nullptr);

	connect(this, &population_unit::type_changed, this, &population_unit::icon_changed);
}

const icon *population_unit::get_icon() const
{
	return this->get_type()->get_phenotype_icon(this->get_phenotype());
}

void population_unit::set_type(const population_type *type)
{
	if (type == this->get_type()) {
		return;
	}

	this->get_province()->get_game_data()->change_population_type_count(this->get_type(), -1);

	this->type = type;

	this->get_province()->get_game_data()->change_population_type_count(this->get_type(), 1);

	emit type_changed();
}

void population_unit::set_culture(const metternich::culture *culture)
{
	if (culture == this->get_culture()) {
		return;
	}

	this->get_province()->get_game_data()->change_population_culture_count(this->get_culture(), -1);

	this->culture = culture;

	this->get_province()->get_game_data()->change_population_culture_count(this->get_culture(), 1);

	const population_type *culture_population_type = culture->get_population_class_type(this->get_type()->get_population_class());
	if (culture_population_type != this->get_type()) {
		this->set_type(culture_population_type);
	}

	emit culture_changed();
}

void population_unit::set_phenotype(const metternich::phenotype *phenotype)
{
	if (phenotype == this->get_phenotype()) {
		return;
	}

	this->phenotype = phenotype;

	emit phenotype_changed();
}

const icon *population_unit::get_small_icon() const
{
	return this->get_type()->get_phenotype_small_icon(this->get_phenotype());
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
