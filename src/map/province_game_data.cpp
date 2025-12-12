#include "metternich.h"

#include "map/province_game_data.h"

#include "character/character.h"
#include "character/character_game_data.h"
#include "character/skill.h"
#include "database/defines.h"
#include "database/preferences.h"
#include "domain/country_economy.h"
#include "domain/country_government.h"
#include "domain/country_military.h"
#include "domain/country_technology.h"
#include "domain/country_turn_data.h"
#include "domain/culture.h"
#include "domain/domain.h"
#include "domain/domain_game_data.h"
#include "economy/commodity.h"
#include "economy/commodity_container.h"
#include "economy/resource.h"
#include "engine_interface.h"
#include "game/event_trigger.h"
#include "game/game.h"
#include "game/province_event.h"
#include "infrastructure/building_type.h"
#include "infrastructure/holding_type.h"
#include "infrastructure/improvement.h"
#include "map/diplomatic_map_mode.h"
#include "map/map.h"
#include "map/province.h"
#include "map/province_attribute.h"
#include "map/province_map_data.h"
#include "map/province_map_mode.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/site_map_data.h"
#include "map/site_type.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "population/population.h"
#include "population/population_type.h"
#include "population/population_unit.h"
#include "religion/religion.h"
#include "script/condition/and_condition.h"
#include "script/context.h"
#include "script/modifier.h"
#include "script/scripted_province_modifier.h"
#include "technology/technology.h"
#include "ui/icon.h"
#include "ui/icon_container.h"
#include "ui/portrait.h"
#include "unit/army.h"
#include "unit/military_unit.h"
#include "unit/military_unit_category.h"
#include "unit/military_unit_type.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/dice.h"
#include "util/image_util.h"
#include "util/log_util.h"
#include "util/map_util.h"
#include "util/point_util.h"
#include "util/vector_random_util.h"

#include "xbrz.h"

#include <magic_enum/magic_enum.hpp>

namespace metternich {

province_game_data::province_game_data(const metternich::province *province)
	: province(province)
{
	this->population = make_qunique<metternich::population>();
	connect(this->get_population(), &population::main_culture_changed, this, &province_game_data::on_population_main_culture_changed);
	connect(this->get_population(), &population::main_religion_changed, this, &province_game_data::on_population_main_religion_changed);

	connect(this, &province_game_data::provincial_capital_changed, this, &province_game_data::visible_sites_changed);
}

province_game_data::~province_game_data()
{
}

void province_game_data::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "domain") {
		this->owner = domain::get(value);
	} else if (key == "culture") {
		this->culture = culture::get(value);
	} else if (key == "religion") {
		this->religion = religion::get(value);
	} else if (key == "level") {
		this->level = std::stoi(value);
	} else if (key == "provincial_capital") {
		this->provincial_capital = site::get(value);
	} else {
		throw std::runtime_error(std::format("Invalid province game data property: \"{}\".", key));
	}
}

void province_game_data::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "attributes") {
		scope.for_each_property([&](const gsml_property &attribute_property) {
			this->attribute_values[province_attribute::get(attribute_property.get_key())] = std::stoi(attribute_property.get_value());
		});
	} else {
		throw std::runtime_error(std::format("Invalid province game data scope: \"{}\".", tag));
	}
}

gsml_data province_game_data::to_gsml_data() const
{
	gsml_data data(this->province->get_identifier());

	if (this->get_owner() != nullptr) {
		data.add_property("domain", this->get_owner()->get_identifier());
	}

	if (this->get_culture() != nullptr) {
		data.add_property("culture", this->get_culture()->get_identifier());
	}

	if (this->get_religion() != nullptr) {
		data.add_property("religion", this->get_religion()->get_identifier());
	}

	if (this->get_level() != 0) {
		data.add_property("level", std::to_string(this->get_level()));
	}

	if (this->get_provincial_capital() != 0) {
		data.add_property("provincial_capital", provincial_capital->get_identifier());
	}

	if (!this->attribute_values.empty()) {
		gsml_data attributes_data("attributes");
		for (const auto &[attribute, value] : this->attribute_values) {
			attributes_data.add_property(attribute->get_identifier(), std::to_string(value));
		}
		data.add_child(std::move(attributes_data));
	}

	return data;
}

void province_game_data::do_turn()
{
	for (const site *site : this->get_sites()) {
		assert_throw(site->get_map_data()->is_on_map());
		site->get_game_data()->do_turn();
	}

	this->decrement_scripted_modifiers();
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

int province_game_data::collect_taxes()
{
	assert_throw(this->get_owner() != nullptr);

	const dice &taxation_dice = defines::get()->get_province_taxation_for_level(this->get_level());
	const int taxation = random::get()->roll_dice(taxation_dice) * 200000;
	if (taxation < 0) {
		//ignore negative results
		return 0;
	}

	this->get_owner()->get_game_data()->get_economy()->change_stored_commodity(defines::get()->get_wealth_commodity(), taxation);

	return taxation;
}

void province_game_data::do_military_unit_recruitment()
{
	if (this->get_owner() == nullptr) {
		return;
	}

	try {
		if (this->get_owner()->get_game_data()->is_under_anarchy()) {
			return;
		}

		const military_unit_type_map<int> recruitment_counts = this->military_unit_recruitment_counts;
		if (recruitment_counts.empty()) {
			return;
		}

		for (const auto &[military_unit_type, recruitment_count] : recruitment_counts) {
			assert_throw(recruitment_count > 0);

			for (int i = 0; i < recruitment_count; ++i) {
				const bool created = this->get_owner()->get_military()->create_military_unit(military_unit_type, this->province, nullptr, {});
				const bool restore_costs = !created;
				this->change_military_unit_recruitment_count(military_unit_type, -1, restore_costs);
			}
		}

		assert_throw(this->military_unit_recruitment_counts.empty());

		if (this->get_owner() == game::get()->get_player_country()) {
			std::string recruitment_counts_str;
			for (const auto &[military_unit_type, recruitment_count] : recruitment_counts) {
				recruitment_counts_str += std::format("\n{} {}", recruitment_count, military_unit_type->get_name());
			}

			const portrait *war_minister_portrait = this->get_owner()->get_government()->get_war_minister_portrait();

			engine_interface::get()->add_notification(std::format("Military Units Recruited in {}", this->get_current_cultural_name()), war_minister_portrait, std::format("Your Excellency, we have recruited new military units for our army in {}.\n{}", this->get_current_cultural_name(), recruitment_counts_str));
		}
	} catch (...) {
		std::throw_with_nested(std::runtime_error(std::format("Error doing military unit recruitment for country \"{}\" in province \"{}\".", this->get_owner()->get_identifier(), this->province->get_identifier())));
	}
}

bool province_game_data::is_on_map() const
{
	return this->province->get_map_data()->is_on_map();
}

void province_game_data::set_owner(const domain *domain)
{
	if (domain == this->get_owner()) {
		return;
	}

	const metternich::domain *old_owner = this->owner;

	this->owner = domain;

	for (const site *site : this->get_sites()) {
		if (site->get_game_data()->get_dungeon() != nullptr) {
			//dungeon sites cannot have owners
			continue;
		}

		if (site->get_game_data()->get_owner() == old_owner) {
			site->get_game_data()->set_owner(domain);
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

	//clear military unit recruitment if the owner changes
	this->clear_military_unit_recruitment_counts();

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

void province_game_data::set_level(const int level)
{
	assert_throw(level >= 0);
	assert_throw(level <= this->get_max_level());

	if (level == this->get_level()) {
		return;
	}

	if (this->get_owner() != nullptr) {
		this->get_owner()->get_game_data()->change_economic_score(-this->get_level() * 100);
	}

	this->level = level;

	if (this->get_owner() != nullptr) {
		this->get_owner()->get_game_data()->change_economic_score(this->get_level() * 100);
	}

	if (game::get()->is_running()) {
		emit level_changed();

		if (this->get_owner() != nullptr) {
			emit this->get_owner()->get_game_data()->income_changed();
		}
	}
}

void province_game_data::change_level(const int change)
{
	this->set_level(this->get_level() + change);
}

int province_game_data::get_max_level() const
{
	return this->province->get_map_data()->get_max_level();
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

const site *province_game_data::get_provincial_capital() const
{
	return this->provincial_capital;
}

void province_game_data::set_provincial_capital(const site *site)
{
	if (site == this->get_provincial_capital()) {
		return;
	}

	this->provincial_capital = site;

	if (game::get()->is_running()) {
		emit provincial_capital_changed();
	}
}

void province_game_data::choose_provincial_capital()
{
	std::vector<const site *> potential_provincial_capitals;
	bool found_default_provincial_capital = false;
	int best_holding_level = 0;

	for (const site *site : this->province->get_map_data()->get_settlement_sites()) {
		if (!site->get_game_data()->is_built()) {
			continue;
		}

		if (site->get_game_data()->get_owner() != this->get_owner()) {
			continue;
		}

		if (!site->get_game_data()->get_holding_type()->is_political()) {
			continue;
		}

		if (this->get_owner() != nullptr && site == this->get_owner()->get_default_capital()) {
			potential_provincial_capitals = { site };
			break;
		} else if (site == this->province->get_default_provincial_capital()) {
			potential_provincial_capitals = { site };
			found_default_provincial_capital = true;
		} else if (!found_default_provincial_capital) {
			if (site->get_game_data()->get_holding_level() > best_holding_level) {
				potential_provincial_capitals.clear();
				best_holding_level = site->get_game_data()->get_holding_level();
			} else if (site->get_game_data()->get_holding_level() < best_holding_level) {
				continue;
			}

			potential_provincial_capitals.push_back(site);
		}
	}

	if (!potential_provincial_capitals.empty()) {
		this->set_provincial_capital(vector::get_random(potential_provincial_capitals));
	} else {
		this->set_provincial_capital(nullptr);
	}
}

const site *province_game_data::get_best_provincial_capital_slot() const
{
	assert_throw(this->get_provincial_capital() == nullptr);

	std::vector<const site *> potential_provincial_capitals;
	bool found_default_provincial_capital = false;
	int best_max_holding_level = 0;

	for (const site *site : this->province->get_map_data()->get_settlement_sites()) {
		if (site->get_game_data()->is_built()) {
			continue;
		}

		if (site->get_holding_type() == nullptr) {
			continue;
		}

		if (!site->get_holding_type()->is_political()) {
			continue;
		}

		if (site->get_game_data()->get_owner() != this->get_owner()) {
			continue;
		}

		if (this->get_owner() != nullptr && site == this->get_owner()->get_default_capital()) {
			potential_provincial_capitals = { site };
			break;
		} else if (site == this->province->get_default_provincial_capital()) {
			potential_provincial_capitals = { site };
			found_default_provincial_capital = true;
		} else if (!found_default_provincial_capital) {
			if (site->get_max_holding_level() > best_max_holding_level) {
				potential_provincial_capitals.clear();
				best_max_holding_level = site->get_max_holding_level();
			} else if (site->get_max_holding_level() < best_max_holding_level) {
				continue;
			}

			potential_provincial_capitals.push_back(site);
		}
	}

	if (!potential_provincial_capitals.empty()) {
		return vector::get_random(potential_provincial_capitals);
	} else {
		return nullptr;
	}
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

	const int tile_pixel_size = map::get()->get_province_map_tile_pixel_size();

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

			const QPoint north_pos = pixel_pos + QPoint(0, -1);
			const QPoint east_pos = pixel_pos + QPoint(1, 0);
			const bool is_border_pixel = image.pixelColor(north_pos).alpha() == 0 || image.pixelColor(east_pos).alpha() == 0;

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

	const int tile_pixel_size = map->get_province_map_tile_pixel_size();
	this->map_image_rect = QRect(this->province->get_map_data()->get_territory_rect().topLeft() * tile_pixel_size * preferences::get()->get_scale_factor(), this->map_image.size());

	co_await this->create_map_mode_image(province_map_mode::terrain);
	co_await this->create_map_mode_image(province_map_mode::cultural);
	co_await this->create_map_mode_image(province_map_mode::trade_zone);
	co_await this->create_map_mode_image(province_map_mode::temple);

	this->calculate_text_rect();

	emit map_image_changed();
}

const QImage &province_game_data::get_map_mode_image(const province_map_mode mode) const
{
	const auto find_iterator = this->map_mode_images.find(mode);
	if (find_iterator != this->map_mode_images.end()) {
		return find_iterator->second;
	}

	throw std::runtime_error(std::format("No map image found for mode {}.", magic_enum::enum_name(mode)));
}

QCoro::Task<void> province_game_data::create_map_mode_image(const province_map_mode mode)
{
	static const QColor empty_color(Qt::black);

	const map *map = map::get();

	QImage image = this->prepare_map_image();

	QColor province_color = this->get_map_color();
	switch (mode) {
		case province_map_mode::cultural: {
			const metternich::culture *culture = this->get_culture();
			if (culture != nullptr) {
				province_color = culture->get_color();
			}
			break;
		}
		case province_map_mode::trade_zone: {
			const domain *trade_zone_domain = this->get_trade_zone_domain();
			if (trade_zone_domain != nullptr) {
				province_color = trade_zone_domain->get_color();
			}
			break;
		}
		case province_map_mode::temple: {
			const domain *temple_domain = this->get_temple_domain();
			if (temple_domain != nullptr) {
				province_color = temple_domain->get_color();
			}
			break;
		}
		default:
			break;
	}

	for (int x = 0; x < this->get_territory_rect().width(); ++x) {
		for (int y = 0; y < this->get_territory_rect().height(); ++y) {
			const QPoint relative_tile_pos = QPoint(x, y);
			const tile *tile = map->get_tile(this->get_territory_rect().topLeft() + relative_tile_pos);

			if (tile->get_province() != this->province) {
				continue;
			}

			const QColor *color = nullptr;

			switch (mode) {
				case province_map_mode::terrain:
					color = &tile->get_terrain()->get_color();
					break;
				case province_map_mode::cultural:
				case province_map_mode::trade_zone:
				case province_map_mode::temple:
					color = &province_color;
					break;
				default:
					assert_throw(false);
					break;
			}

			assert_throw(color != nullptr);

			image.setPixelColor(relative_tile_pos, *color);
		}
	}

	this->map_mode_images[mode] = co_await this->finalize_map_image(std::move(image));
}

void province_game_data::calculate_text_rect()
{
	this->text_rect = QRect();

	QPoint center_pos = this->get_territory_rect_center();

	const map *map = map::get();

	if (map->get_tile(center_pos)->get_province() != this->province && this->province->get_default_provincial_capital() != nullptr && this->province->get_default_provincial_capital()->get_map_data()->is_on_map()) {
		center_pos = this->province->get_default_provincial_capital()->get_game_data()->get_tile_pos();

		if (map->get_tile(center_pos)->get_province() != this->province) {
			return;
		}
	}

	this->text_rect = QRect(center_pos, QSize(1, 1));

	bool changed = true;
	while (changed) {
		changed = false;

		bool can_expand_left = true;
		const int left_x = this->text_rect.left() - 1;
		for (int y = this->text_rect.top(); y <= this->text_rect.bottom(); ++y) {
			const QPoint adjacent_pos(left_x, y);

			if (!this->get_territory_rect().contains(adjacent_pos)) {
				can_expand_left = false;
				break;
			}

			const metternich::tile *adjacent_tile = map->get_tile(adjacent_pos);

			if (adjacent_tile->get_province() != this->province) {
				can_expand_left = false;
				break;
			}
		}
		if (can_expand_left) {
			this->text_rect.setLeft(left_x);
			changed = true;
		}

		bool can_expand_right = true;
		const int right_x = this->text_rect.right() + 1;
		for (int y = this->text_rect.top(); y <= this->text_rect.bottom(); ++y) {
			const QPoint adjacent_pos(right_x, y);

			if (!this->get_territory_rect().contains(adjacent_pos)) {
				can_expand_right = false;
				break;
			}

			const metternich::tile *adjacent_tile = map->get_tile(adjacent_pos);

			if (adjacent_tile->get_province() != this->province) {
				can_expand_right = false;
				break;
			}
		}
		if (can_expand_right) {
			this->text_rect.setRight(right_x);
			changed = true;
		}

		bool can_expand_up = true;
		const int up_y = this->text_rect.top() - 1;
		for (int x = this->text_rect.left(); x <= this->text_rect.right(); ++x) {
			const QPoint adjacent_pos(x, up_y);

			if (!this->get_territory_rect().contains(adjacent_pos)) {
				can_expand_up = false;
				break;
			}

			const metternich::tile *adjacent_tile = map->get_tile(adjacent_pos);

			if (adjacent_tile->get_province() != this->province) {
				can_expand_up = false;
				break;
			}
		}
		if (can_expand_up) {
			this->text_rect.setTop(up_y);
			changed = true;
		}

		bool can_expand_down = true;
		const int down_y = this->text_rect.bottom() + 1;
		for (int x = this->text_rect.left(); x <= this->text_rect.right(); ++x) {
			const QPoint adjacent_pos(x, down_y);

			if (!this->get_territory_rect().contains(adjacent_pos)) {
				can_expand_down = false;
				break;
			}

			const metternich::tile *adjacent_tile = map->get_tile(adjacent_pos);

			if (adjacent_tile->get_province() != this->province) {
				can_expand_down = false;
				break;
			}
		}
		if (can_expand_down) {
			this->text_rect.setBottom(down_y);
			changed = true;
		}
	}
}

QVariantList province_game_data::get_attribute_values_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_attribute_values());
}

void province_game_data::change_attribute_value(const province_attribute *attribute, const int change)
{
	if (change == 0) {
		return;
	}

	const int old_value = this->get_attribute_value(attribute);

	const int new_value = (this->attribute_values[attribute] += change);

	if (new_value == 0) {
		this->attribute_values.erase(attribute);
	}

	if (change > 0) {
		for (int i = old_value + 1; i <= new_value; ++i) {
			const modifier<const metternich::province> *value_modifier = attribute->get_value_modifier(i);
			if (value_modifier != nullptr) {
				value_modifier->apply(this->province);
			}
		}
	} else {
		for (int i = old_value; i > new_value; --i) {
			const modifier<const metternich::province> *value_modifier = attribute->get_value_modifier(i);
			if (value_modifier != nullptr) {
				value_modifier->remove(this->province);
			}
		}
	}

	if (game::get()->is_running()) {
		emit attribute_values_changed();
	}
}

bool province_game_data::do_attribute_check(const province_attribute *attribute, const int roll_modifier) const
{
	static constexpr dice check_dice(1, 20);

	const int roll_result = random::get()->roll_dice(check_dice);

	//there should always be at least a 5% chance of failure
	if (roll_result == check_dice.get_sides()) {
		//e.g. if a 20 is rolled for a d20 roll
		return false;
	}

	const int attribute_value = this->get_attribute_value(attribute);
	const int modified_attribute_value = attribute_value + roll_modifier;
	return roll_result <= modified_attribute_value;
}

int province_game_data::get_attribute_check_chance(const province_attribute *attribute, const int roll_modifier) const
{
	assert_throw(attribute != nullptr);

	static constexpr dice check_dice(1, 20);

	int chance = this->get_attribute_value(attribute);
	chance += roll_modifier;

	if (check_dice.get_sides() != 100) {
		chance *= 100;
		chance /= check_dice.get_sides();
	}

	chance = std::min(chance, 95);

	return chance;
}

std::vector<const site *> province_game_data::get_visible_sites() const
{
	std::vector<const site *> visible_sites = this->province->get_map_data()->get_sites();

	std::erase_if(visible_sites, [](const site *site) {
		if (site->get_type() == site_type::holding || (site->get_type() == site_type::dungeon && site->get_game_data()->get_dungeon() != nullptr)) {
			return false;
		}

		return !site->get_game_data()->is_built();
	});

	std::sort(visible_sites.begin(), visible_sites.end(), [](const site *lhs, const site *rhs) {
		if (lhs->get_game_data()->is_provincial_capital() != rhs->get_game_data()->is_provincial_capital()) {
			return lhs->get_game_data()->is_provincial_capital();
		}

		if (lhs->get_game_data()->is_used() != rhs->get_game_data()->is_used()) {
			return lhs->get_game_data()->is_used();
		}

		if (lhs->get_type() != rhs->get_type()) {
			return lhs->get_type() < rhs->get_type();
		}

		return lhs->get_identifier() < rhs->get_identifier();
	});

	return visible_sites;
}

QVariantList province_game_data::get_visible_sites_qvariant_list() const
{
	return container::to_qvariant_list(this->get_visible_sites());
}

const resource_map<int> &province_game_data::get_resource_counts() const
{
	return this->province->get_map_data()->get_resource_counts();
}

const terrain_type_map<int> &province_game_data::get_tile_terrain_counts() const
{
	return this->province->get_map_data()->get_tile_terrain_counts();
}

bool province_game_data::produces_commodity(const commodity *commodity) const
{
	for (const QPoint &tile_pos : this->get_resource_tiles()) {
		const tile *tile = map::get()->get_tile(tile_pos);

		if (tile->produces_commodity(commodity)) {
			return true;
		}
	}

	for (const site *site : this->province->get_map_data()->get_settlement_sites()) {
		if (!site->get_game_data()->is_built()) {
			continue;
		}

		if (site->get_game_data()->produces_commodity(commodity)) {
			return true;
		}
	}

	return false;
}

QVariantList province_game_data::get_technologies_qvariant_list() const
{
	return container::to_qvariant_list(this->get_technologies());
}

void province_game_data::add_technology(const technology *technology)
{
	if (this->has_technology(technology)) {
		return;
	}

	this->technologies.insert(technology);

	if (this->get_owner() != nullptr && this->is_capital()) {
		this->get_owner()->get_technology()->on_technology_added(technology);
	}

	if (game::get()->is_running()) {
		emit technologies_changed();
	}
}

void province_game_data::add_technology_with_prerequisites(const technology *technology)
{
	this->add_technology(technology);

	for (const metternich::technology *prerequisite : technology->get_prerequisites()) {
		this->add_technology_with_prerequisites(prerequisite);
	}
}

void province_game_data::remove_technology(const technology *technology)
{
	assert_throw(technology != nullptr);

	if (!this->has_technology(technology)) {
		return;
	}

	this->technologies.erase(technology);

	if (this->get_owner() != nullptr && this->is_capital()) {
		this->get_owner()->get_technology()->on_technology_lost(technology);
	}

	//remove any technologies requiring this one as well
	for (const metternich::technology *requiring_technology : technology->get_leads_to()) {
		this->remove_technology(requiring_technology);
	}

	if (game::get()->is_running()) {
		emit technologies_changed();
	}
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

QVariantList province_game_data::get_military_units_qvariant_list() const
{
	return container::to_qvariant_list(this->get_military_units());
}

std::vector<military_unit *> province_game_data::get_country_military_units(const domain *domain) const
{
	std::vector<military_unit *> country_military_units = this->get_military_units();

	std::erase_if(country_military_units, [domain](const military_unit *military_unit) {
		return military_unit->get_country() != domain;
	});

	return country_military_units;
}

QVariantList province_game_data::get_country_military_units_qvariant_list(const domain *domain) const
{
	return container::to_qvariant_list(this->get_country_military_units(domain));
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

bool province_game_data::has_country_military_unit(const domain *domain) const
{
	for (const military_unit *military_unit : this->get_military_units()) {
		if (military_unit->get_country() == domain) {
			return true;
		}
	}

	return false;
}

QVariantList province_game_data::get_country_military_unit_category_counts(metternich::domain *domain) const
{
	std::map<military_unit_category, int> counts;

	for (const military_unit *military_unit : this->get_country_military_units(domain)) {
		if (!military_unit->is_moving()) {
			++counts[military_unit->get_category()];
		}
	}

	return archimedes::map::to_qvariant_list(counts);
}

int province_game_data::get_country_military_unit_category_count(const metternich::military_unit_category category, metternich::domain *domain) const
{
	int count = 0;

	for (const military_unit *military_unit : this->get_military_units()) {
		if (military_unit->get_category() == category && military_unit->get_country() == domain && !military_unit->is_moving()) {
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

const icon *province_game_data::get_country_military_unit_icon(metternich::domain *domain) const
{
	icon_map<int> icon_counts;

	for (const military_unit *military_unit : this->get_military_units()) {
		if (military_unit->get_country() == domain && !military_unit->is_moving()) {
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

const std::vector<military_unit_category> &province_game_data::get_recruitable_military_unit_categories() const
{
	static const std::vector<military_unit_category> recruitable_military_unit_categories{ military_unit_category::light_infantry, military_unit_category::regular_infantry, military_unit_category::heavy_infantry, military_unit_category::light_cavalry, military_unit_category::heavy_cavalry, military_unit_category::light_artillery, military_unit_category::heavy_artillery };

	return recruitable_military_unit_categories;
}

QVariantList province_game_data::get_recruitable_military_unit_categories_qvariant_list() const
{
	return container::to_qvariant_list(this->get_recruitable_military_unit_categories());
}

const military_unit_type_map<int> &province_game_data::get_military_unit_recruitment_counts() const
{
	return this->military_unit_recruitment_counts;
}

QVariantList province_game_data::get_military_unit_recruitment_counts_qvariant_list() const
{
	return container::to_qvariant_list(this->get_military_unit_recruitment_counts());
}

void province_game_data::change_military_unit_recruitment_count(const military_unit_type *military_unit_type, const int change, const bool change_input_storage)
{
	if (change == 0) {
		return;
	}

	const int count = (this->military_unit_recruitment_counts[military_unit_type] += change);

	assert_throw(count >= 0);

	if (count == 0) {
		this->military_unit_recruitment_counts.erase(military_unit_type);
	}

	if (change_input_storage) {
		assert_throw(this->get_owner() != nullptr);

		const int old_count = count - change;
		const commodity_map<int> old_commodity_costs = this->get_owner()->get_military()->get_military_unit_type_commodity_costs(military_unit_type, old_count);
		const commodity_map<int> new_commodity_costs = this->get_owner()->get_military()->get_military_unit_type_commodity_costs(military_unit_type, count);

		for (const auto &[commodity, cost] : new_commodity_costs) {
			assert_throw(commodity->is_storable());

			const int cost_change = cost - old_commodity_costs.find(commodity)->second;

			this->get_owner()->get_economy()->change_stored_commodity(commodity, -cost_change);
		}
	}

	if (game::get()->is_running()) {
		emit military_unit_recruitment_counts_changed();
	}
}

bool province_game_data::can_increase_military_unit_recruitment(const military_unit_type *military_unit_type) const
{
	if (this->get_owner() == nullptr) {
		return false;
	}

	if (this->get_owner()->get_military()->get_best_military_unit_category_type(military_unit_type->get_category()) != military_unit_type) {
		return false;
	}

	const int old_count = this->get_military_unit_recruitment_count(military_unit_type);
	const int new_count = old_count + 1;
	const commodity_map<int> old_commodity_costs = this->get_owner()->get_military()->get_military_unit_type_commodity_costs(military_unit_type, old_count);
	const commodity_map<int> new_commodity_costs = this->get_owner()->get_military()->get_military_unit_type_commodity_costs(military_unit_type, new_count);

	for (const auto &[commodity, cost] : new_commodity_costs) {
		assert_throw(commodity->is_storable());

		const int cost_change = cost - old_commodity_costs.find(commodity)->second;

		if (this->get_owner()->get_economy()->get_stored_commodity(commodity) < cost_change) {
			return false;
		}
	}

	return true;
}

void province_game_data::increase_military_unit_recruitment(const military_unit_type *military_unit_type)
{
	try {
		assert_throw(this->can_increase_military_unit_recruitment(military_unit_type));

		this->change_military_unit_recruitment_count(military_unit_type, 1);
	} catch (...) {
		std::throw_with_nested(std::runtime_error(std::format("Error increasing recruitment of the \"{}\" military unit type for country \"{}\" in province \"{}\".", military_unit_type->get_identifier(), this->get_owner()->get_identifier(), this->province->get_identifier())));
	}
}

bool province_game_data::can_decrease_military_unit_recruitment(const military_unit_type *military_unit_type) const
{
	if (this->get_military_unit_recruitment_count(military_unit_type) == 0) {
		return false;
	}

	return true;
}

void province_game_data::decrease_military_unit_recruitment(const military_unit_type *military_unit_type, const bool restore_inputs)
{
	try {
		assert_throw(this->can_decrease_military_unit_recruitment(military_unit_type));

		this->change_military_unit_recruitment_count(military_unit_type, -1, restore_inputs);
	} catch (...) {
		std::throw_with_nested(std::runtime_error(std::format("Error decreasing recruitment of the \"{}\" military unit type for country \"{}\" in province \"{}\".", military_unit_type->get_identifier(), this->get_owner()->get_identifier(), this->province->get_identifier())));
	}
}

void province_game_data::clear_military_unit_recruitment_counts()
{
	this->military_unit_recruitment_counts.clear();
	emit military_unit_recruitment_counts_changed();
}

void province_game_data::calculate_site_commodity_outputs()
{
	for (const site *site : this->get_sites()) {
		if (!site->is_settlement() && site->get_type() != site_type::resource && site->get_type() != site_type::celestial_body) {
			continue;
		}

		site->get_game_data()->calculate_commodity_outputs();
	}
}

void province_game_data::calculate_site_commodity_output(const commodity *commodity)
{
	for (const site *site : this->get_sites()) {
		if (!site->is_settlement() && site->get_type() != site_type::resource && site->get_type() != site_type::celestial_body) {
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

int province_game_data::get_min_income() const
{
	const dice &taxation_dice = defines::get()->get_province_taxation_for_level(this->get_level());
	return std::max(0, taxation_dice.get_minimum_result() * 200000);
}

int province_game_data::get_max_income() const
{
	const dice &taxation_dice = defines::get()->get_province_taxation_for_level(this->get_level());
	return taxation_dice.get_maximum_result() * 200000;
}

int province_game_data::get_skill_modifier(const skill *skill) const
{
	int modifier = 0;

	for (const auto &[attribute, value] : this->get_attribute_values()) {
		if (attribute->affects_skill(skill)) {
			modifier += value;
		}
	}

	if (skill->get_check_dice().get_sides() != 20) {
		modifier *= skill->get_check_dice().get_sides();
		modifier /= 20;
	}

	return modifier;
}

const domain *province_game_data::get_trade_zone_domain() const
{
	domain_map<int> domain_economic_holding_levels;
	for (const site *holding_site : this->get_settlement_sites()) {
		if (holding_site->get_game_data()->get_owner() == nullptr) {
			continue;
		}

		if (holding_site->get_game_data()->get_holding_type() == nullptr || !holding_site->get_game_data()->get_holding_type()->is_economic()) {
			continue;
		}

		domain_economic_holding_levels[holding_site->get_game_data()->get_owner()] += holding_site->get_game_data()->get_holding_level();
	}

	int best_holding_level = -1;
	const domain *best_domain = nullptr;
	for (const auto &[domain, holding_level] : domain_economic_holding_levels) {
		if (holding_level > best_holding_level) {
			best_holding_level = holding_level;
			best_domain = domain;
		}
	}

	if (best_domain != nullptr) {
		return best_domain;
	}

	return this->get_owner();
}

const domain *province_game_data::get_temple_domain() const
{
	domain_map<int> domain_religious_holding_levels;
	for (const site *holding_site : this->get_settlement_sites()) {
		if (holding_site->get_game_data()->get_owner() == nullptr) {
			continue;
		}

		if (holding_site->get_game_data()->get_holding_type() == nullptr || !holding_site->get_game_data()->get_holding_type()->is_religious()) {
			continue;
		}

		domain_religious_holding_levels[holding_site->get_game_data()->get_owner()] += holding_site->get_game_data()->get_holding_level();
	}

	int best_holding_level = -1;
	const domain *best_domain = nullptr;
	for (const auto &[domain, holding_level] : domain_religious_holding_levels) {
		if (holding_level > best_holding_level) {
			best_holding_level = holding_level;
			best_domain = domain;
		}
	}

	if (best_domain != nullptr) {
		return best_domain;
	}

	return this->get_owner();
}

}
