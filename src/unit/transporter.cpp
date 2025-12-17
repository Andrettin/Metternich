#include "metternich.h"

#include "unit/transporter.h"

#include "domain/country_economy.h"
#include "domain/cultural_group.h"
#include "domain/culture.h"
#include "domain/domain.h"
#include "domain/domain_game_data.h"
#include "language/name_generator.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "ui/icon.h"
#include "unit/military_unit.h" //for the hit point and morale recovery constants
#include "unit/transporter_class.h"
#include "unit/transporter_stat.h"
#include "unit/transporter_type.h"
#include "util/assert_util.h"
#include "util/log_util.h"
#include "util/map_util.h"

namespace metternich {

transporter::transporter(const transporter_type *type, const metternich::domain *domain, const metternich::phenotype *phenotype)
	: type(type), domain(domain), phenotype(phenotype)
{
	assert_throw(this->get_type() != nullptr);
	assert_throw(this->get_country() != nullptr);
	assert_throw(this->get_phenotype() != nullptr);

	this->max_hit_points = type->get_hit_points();
	this->set_hit_points(this->get_max_hit_points());
	this->set_morale(this->get_hit_points());

	for (int i = 0; i < static_cast<int>(transporter_stat::count); ++i) {
		const transporter_stat stat = static_cast<transporter_stat>(i);
		const centesimal_int type_stat_value = type->get_stat_for_country(stat, this->get_country());
		this->change_stat(stat, type_stat_value - type->get_stat(stat));
	}

	this->generate_name();

	connect(this, &transporter::type_changed, this, &transporter::icon_changed);
}

void transporter::do_turn()
{
	const int missing_hit_points = this->get_max_hit_points() - this->get_hit_points();
	assert_throw(missing_hit_points >= 0);
	if (missing_hit_points > 0) {
		//recover unit HP if it is not moving
		this->change_hit_points(std::min(military_unit::hit_point_recovery_per_turn, missing_hit_points));
	}

	const int missing_morale = this->get_hit_points() - this->get_morale();
	assert_throw(missing_morale >= 0);
	if (missing_morale > 0) {
		this->change_morale(std::min(military_unit::morale_recovery_per_turn, missing_morale));
	}
}

void transporter::generate_name()
{
	const std::map<std::string, int> &used_name_counts = this->get_country() ? this->get_country()->get_game_data()->get_unit_name_counts() : archimedes::map::empty_string_to_int_map;

	const culture_base *culture = this->get_culture();

	if (culture == nullptr) {
		return;
	}

	this->name = culture->generate_transporter_name(this->get_type(), used_name_counts);

	if (!this->get_name().empty()) {
		log_trace(std::format("Generated name \"{}\" for transporter of type \"{}\" and culture \"{}\".", this->get_name(), this->get_type()->get_identifier(), culture->get_identifier()));
	}
}

void transporter::set_type(const transporter_type *type)
{
	if (type == this->get_type()) {
		return;
	}

	const transporter_type *old_type = this->get_type();

	this->type = type;

	if (type->get_hit_points() != old_type->get_hit_points()) {
		this->change_max_hit_points(type->get_hit_points() - old_type->get_hit_points());
	}

	for (int i = 0; i < static_cast<int>(transporter_stat::count); ++i) {
		const transporter_stat stat = static_cast<transporter_stat>(i);
		const centesimal_int type_stat_value = type->get_stat_for_country(stat, this->get_country());
		const centesimal_int old_type_stat_value = old_type->get_stat_for_country(stat, this->get_country());
		if (type_stat_value != old_type_stat_value) {
			this->change_stat(stat, type_stat_value - old_type_stat_value);
		}
	}

	emit type_changed();
}

transporter_category transporter::get_category() const
{
	return this->get_type()->get_category();
}

bool transporter::is_ship() const
{
	return this->get_type()->is_ship();
}

const icon *transporter::get_icon() const
{
	return this->get_type()->get_icon();
}

const metternich::culture_base *transporter::get_culture() const
{
	if (this->get_country() != nullptr) {
		return this->get_country()->get_game_data()->get_culture();
	}

	if (this->get_type()->get_culture() != nullptr) {
		return this->get_type()->get_culture();
	}

	if (this->get_type()->get_cultural_group() != nullptr) {
		return this->get_type()->get_cultural_group();
	}

	return nullptr;
}

void transporter::set_hit_points(const int hit_points)
{
	if (hit_points == this->get_hit_points()) {
		return;
	}

	this->hit_points = hit_points;

	assert_throw(this->get_hit_points() <= this->get_max_hit_points());

	if (this->get_morale() > this->get_hit_points()) {
		this->set_morale(this->get_hit_points());
	}

	if (this->get_hit_points() <= 0) {
		this->disband(true);
	}
}

int transporter::get_discipline() const
{
	return 0;
}

int transporter::get_cargo() const
{
	return this->get_type()->get_cargo();
}

void transporter::receive_damage(const int damage, const int morale_damage_modifier)
{
	this->change_hit_points(-damage);

	int morale_damage = damage;
	morale_damage *= 100 + morale_damage_modifier;
	morale_damage /= 100 + this->get_discipline();

	this->change_morale(-morale_damage);
}

void transporter::heal(const int healing)
{
	const int missing_hit_points = this->get_max_hit_points() - this->get_hit_points();

	if (missing_hit_points == 0) {
		return;
	}

	this->change_hit_points(std::min(healing, missing_hit_points));
}

void transporter::disband(const bool dead)
{
	Q_UNUSED(dead);

	if (this->get_country() != nullptr) {
		this->get_country()->get_game_data()->remove_transporter(this);
	}
}

void transporter::disband()
{
	this->disband(false);
}

int transporter::get_score() const
{
	return this->get_type()->get_score();
}

}
