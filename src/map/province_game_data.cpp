#include "metternich.h"

#include "map/province_game_data.h"

#include "character/character.h"
#include "character/character_game_data.h"
#include "character/character_role.h"
#include "country/country.h"
#include "country/country_game_data.h"
#include "country/country_government.h"
#include "country/country_military.h"
#include "country/country_technology.h"
#include "country/country_turn_data.h"
#include "database/defines.h"
#include "database/preferences.h"
#include "economy/commodity.h"
#include "economy/commodity_container.h"
#include "economy/resource.h"
#include "engine_interface.h"
#include "game/event_trigger.h"
#include "game/game.h"
#include "game/province_event.h"
#include "infrastructure/building_type.h"
#include "infrastructure/improvement.h"
#include "infrastructure/settlement_building_slot.h"
#include "infrastructure/settlement_type.h"
#include "map/diplomatic_map_mode.h"
#include "map/map.h"
#include "map/province.h"
#include "map/province_map_data.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/site_map_data.h"
#include "map/site_type.h"
#include "map/tile.h"
#include "population/population.h"
#include "population/population_type.h"
#include "population/population_unit.h"
#include "population/profession.h"
#include "religion/religion.h"
#include "script/condition/and_condition.h"
#include "script/context.h"
#include "script/modifier.h"
#include "script/scripted_province_modifier.h"
#include "ui/icon.h"
#include "ui/icon_container.h"
#include "ui/portrait.h"
#include "unit/army.h"
#include "unit/military_unit.h"
#include "unit/military_unit_category.h"
#include "unit/military_unit_type.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/image_util.h"
#include "util/log_util.h"
#include "util/map_util.h"
#include "util/point_util.h"
#include "util/vector_random_util.h"

#include "xbrz.h"

namespace metternich {

province_game_data::province_game_data(const metternich::province *province)
	: province(province)
{
	this->population = make_qunique<metternich::population>();
	connect(this->get_population(), &population::type_count_changed, this, &province_game_data::on_population_type_count_changed);
	connect(this->get_population(), &population::main_culture_changed, this, &province_game_data::on_population_main_culture_changed);
	connect(this->get_population(), &population::main_religion_changed, this, &province_game_data::on_population_main_religion_changed);

	connect(this, &province_game_data::culture_changed, this, &province_game_data::governor_title_name_changed);
	connect(this, &province_game_data::religion_changed, this, &province_game_data::governor_title_name_changed);
	connect(this, &province_game_data::governor_changed, this, &province_game_data::governor_title_name_changed);
}

province_game_data::~province_game_data()
{
}

void province_game_data::do_turn()
{
	this->allocate_population();

	this->check_employment();

	for (const site *site : this->get_sites()) {
		site->get_game_data()->do_turn();
	}

	this->decrement_scripted_modifiers();
}

void province_game_data::do_everyday_consumption()
{
	std::vector<population_unit *> shuffled_population_units;
	for (population_unit *population_unit : this->get_population_units()) {
		shuffled_population_units.push_back(population_unit);
	}
	vector::shuffle(shuffled_population_units);

	std::vector<population_unit *> population_units;
	for (population_unit *population_unit : shuffled_population_units) {
		if (population_unit->is_everyday_consumption_fulfilled()) {
			population_units.push_back(population_unit);
		} else {
			population_units.insert(population_units.begin(), population_unit);
		}
	}

	for (const auto &[commodity, consumption] : this->local_everyday_consumption) {
		assert_throw(commodity->is_local() && commodity->is_provincial());
		assert_throw(!commodity->is_storable());

		if (!commodity->is_enabled()) {
			continue;
		}

		const int effective_consumption = std::min(consumption.to_int(), this->get_local_commodity_output(commodity).to_int());

		centesimal_int remaining_consumption(consumption.to_int() - effective_consumption);
		if (remaining_consumption == 0) {
			continue;
		}

		//go through population units belonging to the settlement in random order, and cause the effects of them not being able to have their consumption fulfilled
		for (population_unit *population_unit : population_units) {
			const centesimal_int pop_consumption = population_unit->get_type()->get_everyday_consumption(commodity);
			if (pop_consumption == 0) {
				continue;
			}

			population_unit->set_everyday_consumption_fulfilled(false);
			remaining_consumption -= pop_consumption;

			if (remaining_consumption <= 0) {
				break;
			}
		}
	}
}

void province_game_data::do_luxury_consumption()
{
	std::vector<population_unit *> shuffled_population_units;
	for (population_unit *population_unit : this->get_population_units()) {
		shuffled_population_units.push_back(population_unit);
	}
	vector::shuffle(shuffled_population_units);

	std::vector<population_unit *> population_units;
	for (population_unit *population_unit : shuffled_population_units) {
		if (population_unit->is_luxury_consumption_fulfilled()) {
			population_units.push_back(population_unit);
		} else {
			population_units.insert(population_units.begin(), population_unit);
		}
	}

	for (const auto &[commodity, consumption] : this->local_luxury_consumption) {
		assert_throw(commodity->is_local() && commodity->is_provincial());
		assert_throw(!commodity->is_storable());

		if (!commodity->is_enabled()) {
			continue;
		}

		const int effective_consumption = std::min(consumption.to_int(), this->get_local_commodity_output(commodity).to_int());

		centesimal_int remaining_consumption(consumption.to_int() - effective_consumption);
		if (remaining_consumption == 0) {
			continue;
		}

		//go through population units belonging to the settlement in random order, and cause the effects of them not being able to have their consumption fulfilled
		for (population_unit *population_unit : population_units) {
			const centesimal_int pop_consumption = population_unit->get_type()->get_luxury_consumption(commodity);
			if (pop_consumption == 0) {
				continue;
			}

			population_unit->set_luxury_consumption_fulfilled(false);
			remaining_consumption -= pop_consumption;

			if (remaining_consumption <= 0) {
				break;
			}
		}
	}
}

void province_game_data::do_events()
{
	const bool is_last_turn_of_year = game::get()->get_year() != game::get()->get_next_date().year();
	if (is_last_turn_of_year) {
		province_event::check_events_for_scope(this->province, event_trigger::yearly_pulse);
	}

	const bool is_last_turn_of_quarter = is_last_turn_of_year || (game::get()->get_date().month() - 1) / 4 != (game::get()->get_next_date().month() - 1) / 4;
	if (is_last_turn_of_quarter) {
		province_event::check_events_for_scope(this->province, event_trigger::quarterly_pulse);
	}

	province_event::check_events_for_scope(this->province, event_trigger::per_turn_pulse);
}

void province_game_data::do_ai_turn()
{
	//visit visitable sites (if any) with military units of this province's owner
	if (this->get_owner() != nullptr && this->has_country_military_unit(this->get_owner())) {
		for (const site *site : this->get_sites()) {
			site_game_data *site_game_data = site->get_game_data();
			if (!site_game_data->can_be_visited()) {
				continue;
			}

			std::vector<military_unit *> military_units = this->get_military_units();

			std::erase_if(military_units, [this](const military_unit *military_unit) {
				if (military_unit->get_country() != this->get_owner()) {
					return true;
				}

				if (military_unit->is_moving()) {
					return true;
				}

				return false;
			});

			if (!military_units.empty()) {
				auto army = make_qunique<metternich::army>(military_units, site);
				this->get_owner()->get_military()->add_army(std::move(army));
			}
			break;
		}
	}
}

bool province_game_data::is_on_map() const
{
	return this->province->get_map_data()->is_on_map();
}

const std::string &province_game_data::get_governor_title_name() const
{
	static const std::string governor_title_name = "Governor";
	//FIXME: add cultural variation and etc. for governor title names
	return governor_title_name;
}

void province_game_data::set_owner(const country *country)
{
	if (country == this->get_owner()) {
		return;
	}

	const metternich::country *old_owner = this->owner;

	this->owner = country;

	for (const site *site : this->get_sites()) {
		if (site->get_game_data()->get_owner() == old_owner) {
			site->get_game_data()->set_owner(country);
		}
	}

	if (old_owner != nullptr) {
		old_owner->get_game_data()->remove_province(this->province);
	}

	if (this->get_owner() != nullptr) {
		this->get_owner()->get_game_data()->add_province(this->province);

		if (this->get_population()->get_main_culture() == nullptr) {
			this->set_culture(this->get_owner()->get_culture());
		}

		if (this->get_population()->get_main_religion() == nullptr) {
			this->set_religion(this->get_owner()->get_game_data()->get_religion());
		}
	} else {
		//remove population if this province becomes unowned
		for (population_unit *population_unit : this->population_units) {
			population_unit->get_site()->get_game_data()->pop_population_unit(population_unit);
		}
	}

	if (game::get()->is_running()) {
		for (const QPoint &tile_pos : this->get_border_tiles()) {
			map::get()->calculate_tile_country_border_directions(tile_pos);
		}

		map::get()->update_minimap_rect(this->get_territory_rect());

		emit owner_changed();
	}
}

bool province_game_data::is_capital() const
{
	if (this->get_owner() == nullptr) {
		return false;
	}

	return this->get_owner()->get_game_data()->get_capital_province() == this->province;
}

void province_game_data::set_culture(const metternich::culture *culture)
{
	if (culture == this->get_culture()) {
		return;
	}

	this->culture = culture;

	if (game::get()->is_running()) {
		if (this->get_owner() != nullptr) {
			this->get_owner()->get_turn_data()->set_diplomatic_map_mode_dirty(diplomatic_map_mode::cultural);
		}
	}

	emit culture_changed();

	for (const site *site : this->get_sites()) {
		if (!site->get_game_data()->can_have_population()) {
			site->get_game_data()->set_culture(culture);
		}
	}

	for (const site *site : this->get_sites()) {
		if (!site->get_game_data()->can_have_population()) {
			continue;
		}

		if (site->get_game_data()->get_population()->get_main_culture() == nullptr) {
			site->get_game_data()->set_culture(this->get_culture());
		}
	}
}

void province_game_data::on_population_main_culture_changed(const metternich::culture *culture)
{
	if (culture != nullptr) {
		this->set_culture(culture);
	} else if (this->get_owner() != nullptr) {
		this->set_culture(this->get_owner()->get_culture());
	} else {
		this->set_culture(nullptr);
	}
}

void province_game_data::set_religion(const metternich::religion *religion)
{
	if (religion == this->get_religion()) {
		return;
	}

	this->religion = religion;

	for (const site *site : this->get_sites()) {
		if (!site->get_game_data()->can_have_population()) {
			continue;
		}

		if (site->get_game_data()->get_population()->get_main_religion() == nullptr) {
			site->get_game_data()->set_religion(this->get_religion());
		}
	}

	if (game::get()->is_running()) {
		if (this->get_owner() != nullptr) {
			this->get_owner()->get_turn_data()->set_diplomatic_map_mode_dirty(diplomatic_map_mode::religious);
		}
	}

	emit religion_changed();
}

void province_game_data::on_population_main_religion_changed(const metternich::religion *religion)
{
	if (religion != nullptr) {
		this->set_religion(religion);
	} else if (this->get_owner() != nullptr) {
		this->set_religion(this->get_owner()->get_game_data()->get_religion());
	} else {
		this->set_religion(nullptr);
	}
}

const std::string &province_game_data::get_current_cultural_name() const
{
	return this->province->get_cultural_name(this->get_culture());
}

bool province_game_data::is_coastal() const
{
	return this->province->get_map_data()->is_coastal();
}

bool province_game_data::is_near_water() const
{
	return this->province->get_map_data()->is_near_water();
}

const QRect &province_game_data::get_territory_rect() const
{
	return this->province->get_map_data()->get_territory_rect();
}

const QPoint &province_game_data::get_territory_rect_center() const
{
	return this->province->get_map_data()->get_territory_rect_center();
}

const std::vector<const metternich::province *> &province_game_data::get_neighbor_provinces() const
{
	return this->province->get_map_data()->get_neighbor_provinces();
}

bool province_game_data::is_country_border_province() const
{
	for (const metternich::province *neighbor_province : this->get_neighbor_provinces()) {
		const province_game_data *neighbor_province_game_data = neighbor_province->get_game_data();
		if (neighbor_province_game_data->get_owner() != this->get_owner()) {
			return true;
		}
	}

	return false;
}

const QPoint &province_game_data::get_center_tile_pos() const
{
	return this->province->get_map_data()->get_center_tile_pos();
}

const std::vector<QPoint> &province_game_data::get_border_tiles() const
{
	return this->province->get_map_data()->get_border_tiles();
}

const std::vector<QPoint> &province_game_data::get_resource_tiles() const
{
	return this->province->get_map_data()->get_resource_tiles();
}

const std::vector<const site *> &province_game_data::get_sites() const
{
	return this->province->get_map_data()->get_sites();
}

const std::vector<const site *> &province_game_data::get_settlement_sites() const
{
	return this->province->get_map_data()->get_settlement_sites();
}

const QColor &province_game_data::get_map_color() const
{
	if (this->get_owner() != nullptr) {
		return this->get_owner()->get_game_data()->get_diplomatic_map_color();
	}

	if (this->province->is_water_zone()) {
		return defines::get()->get_ocean_color();
	} else {
		return defines::get()->get_minor_nation_color();
	}
}

QImage province_game_data::prepare_map_image() const
{
	assert_throw(this->province->get_map_data()->get_territory_rect().width() > 0);
	assert_throw(this->province->get_map_data()->get_territory_rect().height() > 0);

	QImage image(this->province->get_map_data()->get_territory_rect().size(), QImage::Format_RGBA8888);
	image.fill(Qt::transparent);

	return image;
}

QCoro::Task<QImage> province_game_data::finalize_map_image(QImage &&image) const
{
	QImage scaled_image;

	const int tile_pixel_size = map::get()->get_diplomatic_map_tile_pixel_size();

	co_await QtConcurrent::run([tile_pixel_size, &image, &scaled_image]() {
		scaled_image = image::scale<QImage::Format_ARGB32>(image, centesimal_int(tile_pixel_size), [](const size_t factor, const uint32_t *src, uint32_t *tgt, const int src_width, const int src_height) {
			xbrz::scale(factor, src, tgt, src_width, src_height, xbrz::ColorFormat::ARGB);
		});
	});

	image = std::move(scaled_image);

	std::vector<QPoint> border_pixels;

	for (int x = 0; x < image.width(); ++x) {
		for (int y = 0; y < image.height(); ++y) {
			const QPoint pixel_pos(x, y);
			const QColor pixel_color = image.pixelColor(pixel_pos);

			if (pixel_color.alpha() == 0) {
				continue;
			}

			if (pixel_pos.x() == 0 || pixel_pos.y() == 0 || pixel_pos.x() == (image.width() - 1) || pixel_pos.y() == (image.height() - 1)) {
				border_pixels.push_back(pixel_pos);
				continue;
			}

			if (pixel_color.alpha() != 255) {
				//blended color
				border_pixels.push_back(pixel_pos);
				continue;
			}

			bool is_border_pixel = false;
			point::for_each_cardinally_adjacent_until(pixel_pos, [&image, &is_border_pixel](const QPoint &adjacent_pos) {
				if (image.pixelColor(adjacent_pos).alpha() != 0) {
					return false;
				}

				is_border_pixel = true;
				return true;
			});

			if (is_border_pixel) {
				border_pixels.push_back(pixel_pos);
			}
		}
	}

	const QColor &border_pixel_color = defines::get()->get_country_border_color();

	for (const QPoint &border_pixel_pos : border_pixels) {
		image.setPixelColor(border_pixel_pos, border_pixel_color);
	}

	const centesimal_int &scale_factor = preferences::get()->get_scale_factor();

	co_await QtConcurrent::run([&scale_factor, &image, &scaled_image]() {
		scaled_image = image::scale<QImage::Format_ARGB32>(image, scale_factor, [](const size_t factor, const uint32_t *src, uint32_t *tgt, const int src_width, const int src_height) {
			xbrz::scale(factor, src, tgt, src_width, src_height, xbrz::ColorFormat::ARGB);
		});
	});

	co_return scaled_image;
}

QCoro::Task<void> province_game_data::create_map_image()
{
	const map *map = map::get();

	QImage diplomatic_map_image = this->prepare_map_image();
	QImage selected_diplomatic_map_image = diplomatic_map_image;

	const QColor &color = this->get_map_color();
	const QColor &selected_color = defines::get()->get_selected_country_color();

	for (int x = 0; x < this->province->get_map_data()->get_territory_rect().width(); ++x) {
		for (int y = 0; y < this->province->get_map_data()->get_territory_rect().height(); ++y) {
			const QPoint relative_tile_pos = QPoint(x, y);
			const tile *tile = map->get_tile(this->province->get_map_data()->get_territory_rect().topLeft() + relative_tile_pos);

			if (tile->get_province() != this->province) {
				continue;
			}

			diplomatic_map_image.setPixelColor(relative_tile_pos, color);
			selected_diplomatic_map_image.setPixelColor(relative_tile_pos, selected_color);
		}
	}

	this->map_image = co_await this->finalize_map_image(std::move(diplomatic_map_image));
	this->selected_map_image = co_await this->finalize_map_image(std::move(selected_diplomatic_map_image));

	const int tile_pixel_size = map->get_diplomatic_map_tile_pixel_size();
	this->map_image_rect = QRect(this->province->get_map_data()->get_territory_rect().topLeft() * tile_pixel_size * preferences::get()->get_scale_factor(), this->map_image.size());

	emit map_image_changed();
}

bool province_game_data::produces_commodity(const commodity *commodity) const
{
	for (const QPoint &tile_pos : this->get_resource_tiles()) {
		const tile *tile = map::get()->get_tile(tile_pos);

		if (tile->produces_commodity(commodity)) {
			return true;
		}
	}

	if (this->province->get_provincial_capital() != nullptr && this->province->get_provincial_capital()->get_game_data()->produces_commodity(commodity)) {
		return true;
	}

	return false;
}

const resource_map<int> &province_game_data::get_resource_counts() const
{
	return this->province->get_map_data()->get_resource_counts();
}

const terrain_type_map<int> &province_game_data::get_tile_terrain_counts() const
{
	return this->province->get_map_data()->get_tile_terrain_counts();
}

QVariantList province_game_data::get_scripted_modifiers_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_scripted_modifiers());
}

bool province_game_data::has_scripted_modifier(const scripted_province_modifier *modifier) const
{
	return this->get_scripted_modifiers().contains(modifier);
}

void province_game_data::add_scripted_modifier(const scripted_province_modifier *modifier, const int duration)
{
	const read_only_context ctx(this->province);

	this->scripted_modifiers[modifier] = std::max(this->scripted_modifiers[modifier], duration);

	if (modifier->get_modifier() != nullptr) {
		this->apply_modifier(modifier->get_modifier());
	}

	if (game::get()->is_running()) {
		emit scripted_modifiers_changed();
	}
}

void province_game_data::remove_scripted_modifier(const scripted_province_modifier *modifier)
{
	this->scripted_modifiers.erase(modifier);

	if (modifier->get_modifier() != nullptr) {
		this->remove_modifier(modifier->get_modifier());
	}

	if (game::get()->is_running()) {
		emit scripted_modifiers_changed();
	}
}

void province_game_data::decrement_scripted_modifiers()
{
	std::vector<const scripted_province_modifier *> modifiers_to_remove;
	for (auto &[modifier, duration] : this->scripted_modifiers) {
		--duration;

		if (duration == 0) {
			modifiers_to_remove.push_back(modifier);
		}
	}

	for (const scripted_province_modifier *modifier : modifiers_to_remove) {
		this->remove_scripted_modifier(modifier);
	}
}

void province_game_data::apply_modifier(const modifier<const metternich::province> *modifier, const int multiplier)
{
	assert_throw(modifier != nullptr);

	modifier->apply(this->province, multiplier);
}

void province_game_data::add_population_unit(population_unit *population_unit)
{
	this->population_units.push_back(population_unit);

	if (game::get()->is_running()) {
		emit population_units_changed();
	}
}

void province_game_data::remove_population_unit(population_unit *population_unit)
{
	std::erase(this->population_units, population_unit);

	if (game::get()->is_running()) {
		emit population_units_changed();
	}
}

void province_game_data::clear_population_units()
{
	this->population_units.clear();
}


void province_game_data::on_population_type_count_changed(const population_type *type, const int change)
{
	for (const auto &[commodity, value] : type->get_everyday_consumption()) {
		if (!commodity->is_enabled()) {
			continue;
		}

		if (commodity->is_local() && commodity->is_provincial()) {
			this->change_local_everyday_consumption(commodity, value * change);
		}
	}

	for (const auto &[commodity, value] : type->get_luxury_consumption()) {
		if (!commodity->is_enabled()) {
			continue;
		}

		if (commodity->is_local() && commodity->is_provincial()) {
			this->change_local_luxury_consumption(commodity, value * change);
		}
	}
}

void province_game_data::allocate_population()
{
	//move resource site population above housing capacity to the provincial capital
	const metternich::site *provincial_capital = this->province->get_provincial_capital();

	std::vector<const site *> available_resource_sites;

	for (const site *site : this->get_sites()) {
		if (!site->get_game_data()->can_have_population()) {
			continue;
		}

		if (site->get_map_data()->get_type() != site_type::resource && site->get_map_data()->get_type() != site_type::celestial_body) {
			continue;
		}

		if (site->get_game_data()->get_available_housing() >= 1) {
			available_resource_sites.push_back(site);
			continue;
		}

		while (site->get_game_data()->get_available_housing() < 0) {
			assert_throw(site->get_game_data()->is_built());
			population_unit *population_unit = site->get_game_data()->choose_population_unit_for_reallocation();
			assert_throw(population_unit != nullptr);
			population_unit->migrate_to(provincial_capital);
		}
	}

	//move urban workers (if they are free) to resource sites with available capacity
	site_game_data *provincial_capital_game_data = provincial_capital->get_game_data();

	if (!available_resource_sites.empty()) {
		population_unit *population_unit = provincial_capital_game_data->choose_population_unit_for_reallocation();

		while (population_unit != nullptr) {
			const site *chosen_resource_site = vector::get_random(available_resource_sites);

			if (!chosen_resource_site->get_game_data()->has_available_employment_for(population_unit)) {
				std::erase(available_resource_sites, chosen_resource_site);

				if (available_resource_sites.empty()) {
					break;
				}

				continue;
			}

			population_unit->migrate_to(chosen_resource_site);

			if (chosen_resource_site->get_game_data()->get_available_housing() < 1) {
				std::erase(available_resource_sites, chosen_resource_site);

				if (available_resource_sites.empty()) {
					break;
				}
			}

			population_unit = provincial_capital_game_data->choose_population_unit_for_reallocation();
		}
	}
}

void province_game_data::set_governor(const character *governor)
{
	if (governor == this->get_governor()) {
		return;
	}

	const character *old_governor = this->get_governor();

	if (old_governor != nullptr) {
		old_governor->get_game_data()->apply_governor_modifier(this->province, -1);
		old_governor->get_game_data()->set_country(nullptr);
	}

	this->governor = governor;

	if (this->get_governor() != nullptr) {
		this->get_governor()->get_game_data()->apply_governor_modifier(this->province, 1);
		this->get_governor()->get_game_data()->set_country(this->get_owner());
	}

	if (game::get()->is_running()) {
		emit governor_changed();

		if (old_governor != nullptr) {
			emit old_governor->get_game_data()->governor_changed();
		}

		if (governor != nullptr) {
			emit governor->get_game_data()->governor_changed();
		}
	}
}

void province_game_data::check_governor()
{
	if (this->get_owner() == nullptr || this->get_owner()->get_game_data()->is_under_anarchy()) {
		this->set_governor(nullptr);
		return;
	}

	//remove the governor if they have become obsolete
	if (this->get_governor() != nullptr && this->get_governor()->get_obsolescence_technology() != nullptr && this->get_owner()->get_technology()->has_technology(this->get_governor()->get_obsolescence_technology())) {
		if (game::get()->is_running()) {
			if (this->get_owner() == game::get()->get_player_country()) {
				const portrait *interior_minister_portrait = this->get_owner()->get_government()->get_interior_minister_portrait();

				engine_interface::get()->add_notification(std::format("Governor of {} Retired", this->get_current_cultural_name()), interior_minister_portrait, std::format("Your Excellency, after a distinguished career in our service, governor {} of {} has decided to retire.", this->get_governor()->get_full_name(), this->get_current_cultural_name()));
			}
		}

		this->set_governor(nullptr);
		this->get_governor()->get_game_data()->set_dead(true);
	}

	//if the province has no governor, see if there is any character who can become its governor
	if (this->get_governor() == nullptr) {
		std::vector<const character *> potential_governors;

		for (const character *character : this->province->get_governors()) {
			assert_throw(character->has_role(character_role::governor));

			const character_game_data *character_game_data = character->get_game_data();
			if (character_game_data->get_country() != nullptr) {
				continue;
			}

			if (character_game_data->is_dead()) {
				continue;
			}

			if (character->get_required_technology() != nullptr && !this->get_owner()->get_technology()->has_technology(character->get_required_technology())) {
				continue;
			}

			if (character->get_obsolescence_technology() != nullptr && this->get_owner()->get_technology()->has_technology(character->get_obsolescence_technology())) {
				continue;
			}

			if (character->get_conditions() != nullptr && !character->get_conditions()->check(this->get_owner(), read_only_context(this->get_owner()))) {
				continue;
			}

			potential_governors.push_back(character);
		}

		if (!potential_governors.empty()) {
			this->set_governor(vector::get_random(potential_governors));

			if (this->get_owner() == game::get()->get_player_country() && game::get()->is_running()) {
				const portrait *interior_minister_portrait = this->get_owner()->get_government()->get_interior_minister_portrait();

				engine_interface::get()->add_notification(std::format("New Governor of {}", this->get_current_cultural_name()), interior_minister_portrait, std::format("{} has become the new governor of {}!\n\n{}", this->get_governor()->get_full_name(), this->get_current_cultural_name(), this->get_governor()->get_game_data()->get_governor_modifier_string(this->province)));
			}
		}
	}
}

QVariantList province_game_data::get_military_units_qvariant_list() const
{
	return container::to_qvariant_list(this->get_military_units());
}

std::vector<military_unit *> province_game_data::get_country_military_units(const country *country) const
{
	std::vector<military_unit *> country_military_units = this->get_military_units();

	std::erase_if(country_military_units, [country](const military_unit *military_unit) {
		return military_unit->get_country() != country;
	});

	return country_military_units;
}

QVariantList province_game_data::get_country_military_units_qvariant_list(const country *country) const
{
	return container::to_qvariant_list(this->get_country_military_units(country));
}

void province_game_data::add_military_unit(military_unit *military_unit)
{
	this->military_units.push_back(military_unit);

	if (!military_unit->is_moving()) {
		this->change_military_unit_category_count(military_unit->get_category(), 1);
	}

	if (game::get()->is_running()) {
		emit military_units_changed();
	}
}

void province_game_data::remove_military_unit(military_unit *military_unit)
{
	std::erase(this->military_units, military_unit);

	if (!military_unit->is_moving()) {
		this->change_military_unit_category_count(military_unit->get_category(), -1);
	}

	if (game::get()->is_running()) {
		emit military_units_changed();
	}
}

void province_game_data::clear_military_units()
{
	this->military_units.clear();
	this->military_unit_category_counts.clear();
}

QVariantList province_game_data::get_military_unit_category_counts_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->military_unit_category_counts);
}

void province_game_data::change_military_unit_category_count(const military_unit_category category, const int change)
{
	if (change == 0) {
		return;
	}

	const int count = (this->military_unit_category_counts[category] += change);

	assert_throw(count >= 0);

	if (count == 0) {
		this->military_unit_category_counts.erase(category);
	}

	if (game::get()->is_running()) {
		emit military_unit_category_counts_changed();
	}
}

bool province_game_data::has_country_military_unit(const country *country) const
{
	for (const military_unit *military_unit : this->get_military_units()) {
		if (military_unit->get_country() == country) {
			return true;
		}
	}

	return false;
}

QVariantList province_game_data::get_country_military_unit_category_counts(metternich::country *country) const
{
	std::map<military_unit_category, int> counts;

	for (const military_unit *military_unit : this->get_country_military_units(country)) {
		if (!military_unit->is_moving()) {
			++counts[military_unit->get_category()];
		}
	}

	return archimedes::map::to_qvariant_list(counts);
}

int province_game_data::get_country_military_unit_category_count(const metternich::military_unit_category category, metternich::country *country) const
{
	int count = 0;

	for (const military_unit *military_unit : this->get_military_units()) {
		if (military_unit->get_category() == category && military_unit->get_country() == country && !military_unit->is_moving()) {
			++count;
		}
	}

	return count;
}

const icon *province_game_data::get_military_unit_icon() const
{
	icon_map<int> icon_counts;

	for (const military_unit *military_unit : this->get_military_units()) {
		if (!military_unit->is_moving()) {
			++icon_counts[military_unit->get_icon()];
		}
	}

	const icon *best_icon = nullptr;
	int best_icon_count = 0;
	for (const auto &[icon, count] : icon_counts) {
		if (count > best_icon_count) {
			best_icon = icon;
			best_icon_count = count;
		}
	}

	assert_throw(best_icon != nullptr);

	return best_icon;
}

const icon *province_game_data::get_military_unit_category_icon(const military_unit_category category) const
{
	icon_map<int> icon_counts;

	for (const military_unit *military_unit : this->military_units) {
		if (military_unit->get_category() != category) {
			continue;
		}

		++icon_counts[military_unit->get_icon()];
	}

	const icon *best_icon = nullptr;
	int best_icon_count = 0;
	for (const auto &[icon, count] : icon_counts) {
		if (count > best_icon_count) {
			best_icon = icon;
			best_icon_count = count;
		}
	}

	assert_throw(best_icon != nullptr);

	return best_icon;
}

QString province_game_data::get_military_unit_category_name(const military_unit_category category) const
{
	std::map<QString, int> name_counts;

	for (const military_unit *military_unit : this->military_units) {
		if (military_unit->get_category() != category) {
			continue;
		}

		++name_counts[military_unit->get_type()->get_name_qstring()];
	}

	QString best_name;
	int best_name_count = 0;
	for (const auto &[name, count] : name_counts) {
		if (count > best_name_count) {
			best_name = name;
			best_name_count = count;
		}
	}

	assert_throw(!best_name.isEmpty());

	return best_name;
}

const icon *province_game_data::get_country_military_unit_icon(metternich::country *country) const
{
	icon_map<int> icon_counts;

	for (const military_unit *military_unit : this->get_military_units()) {
		if (military_unit->get_country() == country && !military_unit->is_moving()) {
			++icon_counts[military_unit->get_icon()];
		}
	}

	const icon *best_icon = nullptr;
	int best_icon_count = 0;
	for (const auto &[icon, count] : icon_counts) {
		if (count > best_icon_count) {
			best_icon = icon;
			best_icon_count = count;
		}
	}

	assert_throw(best_icon != nullptr);

	return best_icon;
}

QVariantList province_game_data::get_entering_armies_qvariant_list() const
{
	return container::to_qvariant_list(this->get_entering_armies());
}

void province_game_data::calculate_site_commodity_outputs()
{
	for (const site *site : this->get_sites()) {
		if (!site->is_settlement() && site->get_map_data()->get_type() != site_type::resource && site->get_map_data()->get_type() != site_type::celestial_body) {
			continue;
		}

		site->get_game_data()->calculate_commodity_outputs();
	}
}

void province_game_data::calculate_site_commodity_output(const commodity *commodity)
{
	for (const site *site : this->get_sites()) {
		if (!site->is_settlement() && site->get_map_data()->get_type() != site_type::resource && site->get_map_data()->get_type() != site_type::celestial_body) {
			continue;
		}

		if (!site->get_game_data()->produces_commodity(commodity) && !site->get_game_data()->get_base_commodity_outputs().contains(commodity)) {
			continue;
		}

		site->get_game_data()->calculate_commodity_outputs();
	}
}

void province_game_data::change_local_commodity_output(const commodity *commodity, const centesimal_int &change)
{
	if (change == 0) {
		return;
	}

	const centesimal_int &output = (this->local_commodity_outputs[commodity] += change);

	if (output == 0) {
		this->local_commodity_outputs.erase(commodity);
	}
}

void province_game_data::change_local_everyday_consumption(const commodity *commodity, const centesimal_int &change)
{
	if (change == 0) {
		return;
	}

	log_trace(std::format("Changing local everyday consumption in province {} of commodity {} (currently {}) by {}.", this->province->get_identifier(), commodity->get_identifier(), this->get_local_everyday_consumption(commodity).to_string(), change.to_string()));

	const centesimal_int count = (this->local_everyday_consumption[commodity] += change);

	log_trace(std::format("Changed local everyday consumption in province {} of commodity {} by {}, making it now {}.", this->province->get_identifier(), commodity->get_identifier(), change.to_string(), this->get_local_everyday_consumption(commodity).to_string()));

	assert_throw(count >= 0);

	if (count == 0) {
		this->local_everyday_consumption.erase(commodity);
	}
}

void province_game_data::change_local_luxury_consumption(const commodity *commodity, const centesimal_int &change)
{
	if (change == 0) {
		return;
	}

	log_trace(std::format("Changing local luxury consumption in settlement {} of commodity {} (currently {}) by {}.", this->province->get_identifier(), commodity->get_identifier(), this->get_local_luxury_consumption(commodity).to_string(), change.to_string()));

	const centesimal_int count = (this->local_luxury_consumption[commodity] += change);

	log_trace(std::format("Changed local luxury consumption in settlement {} of commodity {} by {}, making it now {}.", this->province->get_identifier(), commodity->get_identifier(), change.to_string(), this->get_local_luxury_consumption(commodity).to_string()));

	assert_throw(count >= 0);

	if (count == 0) {
		this->local_luxury_consumption.erase(commodity);
	}
}

void province_game_data::change_improved_resource_commodity_bonus(const resource *resource, const commodity *commodity, const int change)
{
	if (change == 0) {
		return;
	}

	const int count = (this->improved_resource_commodity_bonuses[resource][commodity] += change);

	assert_throw(count >= 0);

	if (count == 0) {
		this->improved_resource_commodity_bonuses[resource].erase(commodity);

		if (this->improved_resource_commodity_bonuses[resource].empty()) {
			this->improved_resource_commodity_bonuses.erase(resource);
		}
	}

	for (const QPoint &tile_pos : this->get_resource_tiles()) {
		tile *tile = map::get()->get_tile(tile_pos);
		if (tile->get_resource() != resource) {
			continue;
		}

		assert_throw(tile->get_site() != nullptr);

		if (tile->get_site()->get_game_data()->get_resource_improvement() == nullptr) {
			continue;
		}

		tile->get_site()->get_game_data()->change_base_commodity_output(commodity, centesimal_int(change));
	}
}

void province_game_data::set_commodity_bonus_for_tile_threshold(const commodity *commodity, const int threshold, const int value)
{
	const int old_value = this->get_commodity_bonus_for_tile_threshold(commodity, threshold);

	if (value == old_value) {
		return;
	}

	if (value == 0) {
		this->commodity_bonuses_for_tile_thresholds[commodity].erase(threshold);

		if (this->commodity_bonuses_for_tile_thresholds[commodity].empty()) {
			this->commodity_bonuses_for_tile_thresholds.erase(commodity);
		}
	} else {
		this->commodity_bonuses_for_tile_thresholds[commodity][threshold] = value;
	}

	for (const QPoint &tile_pos : this->get_resource_tiles()) {
		tile *tile = map::get()->get_tile(tile_pos);
		if (!tile->produces_commodity(commodity)) {
			continue;
		}

		tile->calculate_commodity_outputs();
	}
}

bool province_game_data::can_produce_commodity(const commodity *commodity) const
{
	assert_throw(commodity != nullptr);

	for (const QPoint &tile_pos : this->get_resource_tiles()) {
		const tile *tile = map::get()->get_tile(tile_pos);
		const metternich::resource *tile_resource = tile->get_resource();
		const metternich::commodity *tile_resource_commodity = tile_resource->get_commodity();

		if (tile_resource_commodity == commodity) {
			return true;
		}
	}

	return false;
}

std::vector<employment_location *> province_game_data::get_employment_locations() const
{
	std::vector<employment_location *> employment_locations;

	for (const QPoint &tile_pos : this->get_resource_tiles()) {
		const tile *tile = map::get()->get_tile(tile_pos);
		const site *resource_site = tile->get_site();
		site_game_data *resource_site_game_data = resource_site->get_game_data();

		if (resource_site_game_data->get_production_capacity() > 0) {
			employment_locations.push_back(resource_site_game_data);
		}
	}

	const site_game_data *provincial_capital_game_data = this->province->get_provincial_capital()->get_game_data();

	for (const qunique_ptr<settlement_building_slot> &building_slot : provincial_capital_game_data->get_building_slots()) {
		if (building_slot->get_production_capacity() > 0) {
			employment_locations.push_back(building_slot.get());
		}
	}

	return employment_locations;
}

void province_game_data::check_employment()
{
	for (const site *site : this->get_sites()) {
		if (!site->get_game_data()->is_built()) {
			continue;
		}

		if (!site->get_game_data()->can_have_population()) {
			continue;
		}

		site->get_game_data()->check_employment();
	}
}

bool province_game_data::can_employ(const population_unit *population_unit) const
{
	const std::vector<employment_location *> employment_locations = this->get_employment_locations();

	const population_type *converted_population_type = nullptr;

	for (const employment_location *employment_location : employment_locations) {
		for (const profession *profession : employment_location->get_employment_professions()) {
			if (employment_location->can_employ(population_unit, profession, converted_population_type)) {
				return true;
			}
		}
	}

	return false;
}

}
