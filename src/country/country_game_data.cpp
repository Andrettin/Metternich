#include "metternich.h"

#include "country/country_game_data.h"

#include "character/advisor_category.h"
#include "character/advisor_type.h"
#include "character/character.h"
#include "character/character_game_data.h"
#include "country/country.h"
#include "country/country_type.h"
#include "country/culture.h"
#include "country/diplomacy_state.h"
#include "country/religion.h"
#include "database/defines.h"
#include "database/preferences.h"
#include "economy/commodity.h"
#include "economy/production_type.h"
#include "economy/resource.h"
#include "engine_interface.h"
#include "game/country_event.h"
#include "game/event_trigger.h"
#include "game/game.h"
#include "game/game_rules.h"
#include "infrastructure/building_class.h"
#include "infrastructure/building_slot.h"
#include "infrastructure/building_slot_type.h"
#include "infrastructure/building_type.h"
#include "infrastructure/improvement.h"
#include "map/diplomatic_map_mode.h"
#include "map/map.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "population/phenotype.h"
#include "population/population_type.h"
#include "population/population_unit.h"
#include "script/condition/condition.h"
#include "script/factor.h"
#include "script/modifier.h"
#include "script/opinion_modifier.h"
#include "technology/technology.h"
#include "ui/icon.h"
#include "ui/icon_container.h"
#include "unit/civilian_unit.h"
#include "unit/military_unit.h"
#include "unit/military_unit_class.h"
#include "unit/military_unit_type.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/image_util.h"
#include "util/map_util.h"
#include "util/point_util.h"
#include "util/rect_util.h"
#include "util/size_util.h"
#include "util/thread_pool.h"
#include "util/vector_random_util.h"
#include "util/vector_util.h"

#include "xbrz.h"

namespace metternich {

country_game_data::country_game_data(metternich::country *country) : country(country)
{
	connect(this, &country_game_data::rank_changed, this, &country_game_data::type_name_changed);

	this->initialize_building_slots();
}

country_game_data::~country_game_data()
{
}

void country_game_data::do_turn()
{
	try {
		for (const province *province : this->get_provinces()) {
			province->get_game_data()->do_turn();
		}

		this->do_production();
		this->do_research();
		this->do_population_growth();
		this->do_construction();

		for (const qunique_ptr<civilian_unit> &civilian_unit : this->civilian_units) {
			civilian_unit->do_turn();
		}

		for (const qunique_ptr<military_unit> &military_unit : this->military_units) {
			military_unit->do_turn();
		}

		this->decrement_scripted_modifiers();

		this->do_events();

		if (game::get()->get_rules()->are_advisors_enabled() && this->can_have_advisors()) {
			this->check_advisors();
		}
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Failed to process turn for country \"" + this->country->get_identifier() + "\"."));
	}
}

void country_game_data::do_production()
{
	try {
		//FIXME: add preference for production being automatically assigned for person players
		if (this->is_ai()) {
			this->assign_production();
		}

		for (const auto &[commodity, output] : this->get_commodity_outputs()) {
			if (!commodity->is_storable()) {
				assert_throw(output >= 0);
				continue;
			}

			int final_output = output;

			if (commodity == defines::get()->get_research_commodity() && this->get_current_research() != nullptr) {
				final_output *= 100 + this->get_category_research_modifier(this->get_current_research()->get_category());
				final_output /= 100;
			}

			this->change_stored_commodity(commodity, final_output);
		}

		const std::vector<const commodity *> input_commodities = archimedes::map::get_keys(this->get_commodity_inputs());

		//decrease consumption of commodities for which we no longer have enough in storage
		for (const commodity *commodity : input_commodities) {
			if (!commodity->is_storable()) {
				continue;
			}

			while (this->get_commodity_input(commodity) > this->get_stored_commodity(commodity)) {
				this->decrease_commodity_consumption(commodity, false);
			}
		}

		//reduce inputs from the storage for the next turn (for production this turn it had already been subtracted)
		for (const auto &[commodity, input] : this->get_commodity_inputs()) {
			try {
				if (!commodity->is_storable()) {
					const int output = this->get_commodity_output(commodity);
					if (input > output) {
						throw std::runtime_error(std::format("Input for non-storable commodity \"{}\" ({}) is greater than its output ({}).", commodity->get_identifier(), input, output));
					}
					continue;
				}

				this->change_stored_commodity(commodity, -input);
			} catch (...) {
				std::throw_with_nested(std::runtime_error("Error processing input storage reduction for commodity \"" + commodity->get_identifier()  + "\"."));
			}
		}
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Error doing production for country \"" + this->country->get_identifier() + "\"."));
	}
}

void country_game_data::do_research()
{
	try {
		if (this->is_ai()) {
			if (this->get_current_research() == nullptr) {
				this->choose_current_research();
			}
		}

		if (this->get_current_research() == nullptr) {
			return;
		}

		if (this->get_stored_commodity(defines::get()->get_research_commodity()) >= this->get_current_research()->get_cost()) {
			this->add_technology(this->get_current_research());

			if (this->country == game::get()->get_player_country()) {
				const icon *interior_minister_portrait = defines::get()->get_interior_minister_portrait();

				engine_interface::get()->add_notification("Technology Researched", interior_minister_portrait, std::format("Your Excellency, our scholars have made a breakthrough in the research of the {} technology!", this->get_current_research()->get_name()));
			}

			this->set_current_research(nullptr);
		}
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Error doing research for country \"" + this->country->get_identifier() + "\"."));
	}
}

void country_game_data::do_population_growth()
{
	try {
		if (this->get_food_consumption() == 0) {
			this->set_population_growth(0);
			return;
		}

		//this is a copy because we may need to erase elements from the map in the subsequent code
		const commodity_map<int> stored_commodities = this->get_stored_commodities();

		int stored_food = 0;
		for (const auto &[commodity, quantity] : stored_commodities) {
			if (commodity->is_food()) {
				stored_food += quantity;
				this->set_stored_commodity(commodity, 0);
			}
		}

		int food_consumption = this->get_food_consumption();

		for (const province *province : this->get_provinces()) {
			food_consumption -= province->get_game_data()->get_free_food_consumption();
		}

		const int net_food = stored_food - food_consumption;

		this->change_population_growth(net_food);

		while (this->get_population_growth() >= defines::get()->get_population_growth_threshold()) {
			this->grow_population();
		}

		int starvation_count = 0;

		while (this->get_population_growth() < 0) {
			//starvation
			this->decrease_population();
			++starvation_count;

			if (this->get_food_consumption() == 0) {
				this->set_population_growth(0);
				break;
			}
		}

		if (starvation_count > 0 && this->country == game::get()->get_player_country()) {
			const bool plural = starvation_count > 1;

			const icon *interior_minister_portrait = defines::get()->get_interior_minister_portrait();

			engine_interface::get()->add_notification("Starvation", interior_minister_portrait, std::format("Your Excellency, I regret to inform you that {} {} of our population {} starved to death.", number::to_formatted_string(starvation_count), (plural ? "units" : "unit"), (plural ? "have" : "has")));
		}
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Error doing population growth for country \"" + this->country->get_identifier() + "\"."));
	}
}

void country_game_data::do_cultural_change()
{
	static constexpr int cultural_derivation_chance = 1;

	for (const qunique_ptr<population_unit> &population_unit : this->population_units) {
		const metternich::culture *current_culture = population_unit->get_culture();

		std::vector<const metternich::culture *> potential_cultures;

		const read_only_context ctx(population_unit.get());

		for (const metternich::culture *culture : current_culture->get_derived_cultures()) {
			if (culture->get_derivation_conditions() != nullptr && !culture->get_derivation_conditions()->check(population_unit.get(), ctx)) {
				continue;
			}

			potential_cultures.push_back(culture);
		}

		if (!potential_cultures.empty() && random::get()->generate(100) < cultural_derivation_chance) {
			const metternich::culture *new_culture = vector::get_random(potential_cultures);
			population_unit->set_culture(new_culture);
		}
	}
}

void country_game_data::do_construction()
{
	try {
		for (const qunique_ptr<building_slot> &building_slot : this->building_slots) {
			if (building_slot->is_expanding()) {
				building_slot->expand();
			}
		}
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Error doing construction for country \"" + this->country->get_identifier() + "\"."));
	}
}

void country_game_data::do_events()
{
	for (const province *province : this->get_provinces()) {
		province->get_game_data()->do_events();
	}

	const bool is_last_turn_of_year = (game::get()->get_date().date().month() + defines::get()->get_months_per_turn()) > 12;

	if (is_last_turn_of_year) {
		country_event::check_events_for_scope(this->country, event_trigger::yearly_pulse);
	}

	country_event::check_events_for_scope(this->country, event_trigger::quarterly_pulse);
}

void country_game_data::do_ai_turn()
{
	for (const province *province : this->get_provinces()) {
		province->get_game_data()->do_ai_turn();
	}

	for (const qunique_ptr<civilian_unit> &civilian_unit : this->civilian_units) {
		civilian_unit->do_ai_turn();
	}

	for (const qunique_ptr<military_unit> &military_unit : this->military_units) {
		military_unit->do_ai_turn();
	}

	for (size_t i = 0; i < this->civilian_units.size();) {
		civilian_unit *civilian_unit = this->civilian_units.at(i).get();
		if (civilian_unit->is_busy()) {
			++i;
		} else {
			//if the civilian unit is idle, this means that nothing was found for it to do above; in that case, disband it
			civilian_unit->disband();
		}
	}
}

bool country_game_data::is_ai() const
{
	return this->country != game::get()->get_player_country();
}

void country_game_data::set_religion(const metternich::religion *religion)
{
	if (religion == this->get_religion()) {
		return;
	}

	this->religion = religion;

	if (game::get()->is_running()) {
		emit religion_changed();
	}
}

void country_game_data::set_overlord(const metternich::country *overlord)
{
	if (overlord == this->get_overlord()) {
		return;
	}

	if (this->overlord != nullptr) {
		this->overlord->get_game_data()->change_score(-this->get_score() * country::vassal_score_percent / 100);

		for (const auto &[resource, count] : this->get_resource_counts()) {
			this->get_overlord()->get_game_data()->change_vassal_resource_count(resource, -count);
		}
	}

	this->overlord = overlord;

	if (this->overlord != nullptr) {
		this->overlord->get_game_data()->change_score(this->get_score() * country::vassal_score_percent / 100);

		for (const auto &[resource, count] : this->get_resource_counts()) {
			this->get_overlord()->get_game_data()->change_vassal_resource_count(resource, count);
		}
	}

	if (game::get()->is_running()) {
		emit overlord_changed();
	}
}

bool country_game_data::is_vassal_of(const metternich::country *country) const
{
	return this->get_overlord() == country;
}

bool country_game_data::is_any_vassal_of(const metternich::country *country) const
{
	if (this->is_vassal_of(country)) {
		return true;
	}

	if (this->get_overlord() != nullptr) {
		return this->get_overlord()->get_game_data()->is_any_vassal_of(country);
	}

	return false;
}

bool country_game_data::is_overlord_of(const metternich::country *country) const
{
	return country->get_game_data()->is_vassal_of(this->country);
}

bool country_game_data::is_any_overlord_of(const metternich::country *country) const
{
	if (this->is_overlord_of(country)) {
		return true;
	}

	if (this->get_overlord() != nullptr) {
		return this->get_overlord()->get_game_data()->is_any_overlord_of(country);
	}

	return false;
}

bool country_game_data::is_true_great_power() const
{
	if (this->country->get_type() != country_type::great_power) {
		return false;
	}

	return this->get_rank() < country::max_great_powers;
}

bool country_game_data::is_secondary_power() const
{
	//a country is a secondary power if its type is great power, and it is not high-ranking enough to be considered a true great power
	if (this->country->get_type() != country_type::great_power) {
		return false;
	}

	return this->get_rank() >= country::max_great_powers;
}

bool country_game_data::is_colony() const
{
	if (this->get_overlord() == nullptr) {
		return false;
	}

	return this->get_diplomacy_state(this->get_overlord()) == diplomacy_state::colony;
}

std::string country_game_data::get_type_name() const
{
	switch (this->country->get_type()) {
		case country_type::great_power:
			if (this->get_overlord() != nullptr) {
				return "Subject Power";
			}

			if (this->is_secondary_power()) {
				return "Secondary Power";
			}

			return "Great Power";
		case country_type::minor_nation:
			return "Minor Nation";
		case country_type::tribe:
			return "Tribe";
		default:
			assert_throw(false);
	}

	return std::string();
}

std::string country_game_data::get_vassalage_type_name() const
{
	if (this->get_overlord() == nullptr) {
		return std::string();
	}

	switch (this->get_diplomacy_state(this->get_overlord())) {
		case diplomacy_state::vassal:
			return "Vassal";
		case diplomacy_state::personal_union_subject:
			return "Personal Union Subject";
		case diplomacy_state::colony:
			return "Colony";
		default:
			assert_throw(false);
	}

	return std::string();
}

QVariantList country_game_data::get_provinces_qvariant_list() const
{
	return container::to_qvariant_list(this->get_provinces());
}

void country_game_data::add_province(const province *province)
{
	this->provinces.push_back(province);

	map *map = map::get();
	const province_game_data *province_game_data = province->get_game_data();

	this->change_score(province_game_data->get_score());

	if (province_game_data->is_coastal()) {
		++this->coastal_province_count;
	}

	for (const auto &[resource, count] : province_game_data->get_resource_counts()) {
		this->change_resource_count(resource, count);

		if (this->get_overlord() != nullptr) {
			this->get_overlord()->get_game_data()->change_vassal_resource_count(resource, count);
		}
	}

	for (const QPoint &tile_pos : province_game_data->get_resource_tiles()) {
		const tile *tile = map::get()->get_tile(tile_pos);
		const improvement *improvement = tile->get_improvement();

		if (improvement != nullptr && improvement->get_output_commodity() != nullptr) {
			this->change_commodity_output(improvement->get_output_commodity(), improvement->get_output_multiplier());
		}
	}

	for (const auto &[terrain, count] : province_game_data->get_tile_terrain_counts()) {
		this->change_tile_terrain_count(terrain, count);
	}

	if (province_game_data->is_country_border_province()) {
		this->border_provinces.push_back(province);
	}

	for (const metternich::province *neighbor_province : province_game_data->get_neighbor_provinces()) {
		const metternich::province_game_data *neighbor_province_game_data = neighbor_province->get_game_data();
		if (neighbor_province_game_data->get_owner() != this->country) {
			continue;
		}

		//province ceased to be a country border province, remove it from the list
		if (vector::contains(this->get_border_provinces(), neighbor_province) && !neighbor_province_game_data->is_country_border_province()) {
			std::erase(this->border_provinces, neighbor_province);
		}

		for (const QPoint &tile_pos : neighbor_province_game_data->get_border_tiles()) {
			if (!map->is_tile_on_country_border(tile_pos)) {
				std::erase(this->border_tiles, tile_pos);
			}

			if (game::get()->is_running()) {
				map->calculate_tile_country_border_directions(tile_pos);
			}
		}
	}

	for (const QPoint &tile_pos : province_game_data->get_border_tiles()) {
		if (map->is_tile_on_country_border(tile_pos)) {
			this->border_tiles.push_back(tile_pos);
		}
	}

	this->calculate_territory_rect();

	if (this->get_provinces().size() == 1) {
		game::get()->add_country(this->country);
	}

	if (game::get()->is_running()) {
		emit provinces_changed();
	}
}

void country_game_data::remove_province(const province *province)
{
	std::erase(this->provinces, province);

	map *map = map::get();
	const province_game_data *province_game_data = province->get_game_data();

	this->change_score(-province_game_data->get_score());

	if (province_game_data->is_coastal()) {
		--this->coastal_province_count;
	}

	for (const auto &[resource, count] : province_game_data->get_resource_counts()) {
		this->change_resource_count(resource, -count);

		if (this->get_overlord() != nullptr) {
			this->get_overlord()->get_game_data()->change_vassal_resource_count(resource, -count);
		}
	}

	for (const QPoint &tile_pos : province_game_data->get_resource_tiles()) {
		const tile *tile = map::get()->get_tile(tile_pos);
		const improvement *improvement = tile->get_improvement();

		if (improvement != nullptr && improvement->get_output_commodity() != nullptr) {
			this->change_commodity_output(improvement->get_output_commodity(), -improvement->get_output_multiplier());
		}
	}

	for (const auto &[terrain, count] : province_game_data->get_tile_terrain_counts()) {
		this->change_tile_terrain_count(terrain, -count);
	}

	for (const QPoint &tile_pos : province_game_data->get_border_tiles()) {
		std::erase(this->border_tiles, tile_pos);
	}

	std::erase(this->border_provinces, province);

	for (const metternich::province *neighbor_province : province_game_data->get_neighbor_provinces()) {
		const metternich::province_game_data *neighbor_province_game_data = neighbor_province->get_game_data();
		if (neighbor_province_game_data->get_owner() != this->country) {
			continue;
		}

		//province has become a country border province, add it to the list
		if (neighbor_province_game_data->is_country_border_province() && !vector::contains(this->get_border_provinces(), neighbor_province)) {
			this->border_provinces.push_back(neighbor_province);
		}

		for (const QPoint &tile_pos : neighbor_province_game_data->get_border_tiles()) {
			if (map->is_tile_on_country_border(tile_pos) && !vector::contains(this->get_border_tiles(), tile_pos)) {
				this->border_tiles.push_back(tile_pos);
			}

			if (game::get()->is_running()) {
				map->calculate_tile_country_border_directions(tile_pos);
			}
		}
	}

	this->calculate_territory_rect();

	if (this->get_provinces().empty()) {
		game::get()->remove_country(this->country);
	}

	if (game::get()->is_running()) {
		emit provinces_changed();
	}
}

bool country_game_data::is_under_anarchy() const
{
	return this->country->get_capital_province()->get_game_data()->get_owner() != this->country;
}

void country_game_data::calculate_territory_rect()
{
	QRect territory_rect;
	this->contiguous_territory_rects.clear();

	for (const province *province : this->get_provinces()) {
		const province_game_data *province_game_data = province->get_game_data();

		if (territory_rect.isNull()) {
			territory_rect = province_game_data->get_territory_rect();
		} else {
			territory_rect = territory_rect.united(province_game_data->get_territory_rect());
		}

		this->contiguous_territory_rects.push_back(province_game_data->get_territory_rect());
	}

	bool changed = true;
	while (changed) {
		changed = false;

		for (size_t i = 0; i < this->contiguous_territory_rects.size(); ++i) {
			QRect &first_territory_rect = this->contiguous_territory_rects.at(i);

			for (size_t j = i + 1; j < this->contiguous_territory_rects.size();) {
				const QRect &second_territory_rect = this->contiguous_territory_rects.at(j);

				if (first_territory_rect.intersects(second_territory_rect) || rect::is_adjacent_to(first_territory_rect, second_territory_rect)) {
					first_territory_rect = first_territory_rect.united(second_territory_rect);
					this->contiguous_territory_rects.erase(this->contiguous_territory_rects.begin() + j);
					changed = true;
				} else {
					++j;
				}
			}
		}
	}

	this->territory_rect = territory_rect;

	const QPoint &capital_pos = this->country->get_capital_province()->get_capital_settlement()->get_game_data()->get_tile_pos();
	int best_distance = std::numeric_limits<int>::max();
	for (const QRect &contiguous_territory_rect : this->get_contiguous_territory_rects()) {
		if (contiguous_territory_rect.contains(capital_pos)) {
			this->main_contiguous_territory_rect = contiguous_territory_rect;
			break;
		}

		int distance = rect::distance_to(contiguous_territory_rect, capital_pos);

		if (distance < best_distance) {
			best_distance = distance;
			this->main_contiguous_territory_rect = contiguous_territory_rect;
		}
	}

	this->calculate_territory_rect_center();
	this->calculate_text_rect();

	if (game::get()->is_running()) {
		thread_pool::get()->co_spawn_sync([this]() -> boost::asio::awaitable<void> {
			co_await this->create_diplomatic_map_image();
		});
	}
}

void country_game_data::calculate_territory_rect_center()
{
	if (this->get_provinces().empty()) {
		return;
	}

	QPoint sum(0, 0);
	int tile_count = 0;

	for (const province *province : this->get_provinces()) {
		const province_game_data *province_game_data = province->get_game_data();
		if (!this->get_main_contiguous_territory_rect().contains(province_game_data->get_territory_rect())) {
			continue;
		}

		const int province_tile_count = static_cast<int>(province_game_data->get_tiles().size());
		sum += province_game_data->get_territory_rect_center() * province_tile_count;
		tile_count += province_tile_count;
	}

	this->territory_rect_center = QPoint(sum.x() / tile_count, sum.y() / tile_count);
}

QVariantList country_game_data::get_contiguous_territory_rects_qvariant_list() const
{
	return container::to_qvariant_list(this->get_contiguous_territory_rects());
}

void country_game_data::calculate_text_rect()
{
	this->text_rect = QRect();

	if (!this->is_alive()) {
		return;
	}

	QPoint center_pos = this->get_territory_rect_center();

	const map *map = map::get();

	if (map->get_tile(center_pos)->get_owner() != this->country) {
		center_pos = this->country->get_capital_province()->get_capital_settlement()->get_game_data()->get_tile_pos();

		if (map->get_tile(center_pos)->get_owner() != this->country) {
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

			if (!this->main_contiguous_territory_rect.contains(adjacent_pos)) {
				can_expand_left = false;
				break;
			}

			const metternich::tile *adjacent_tile = map->get_tile(adjacent_pos);

			if (adjacent_tile->get_owner() != this->country) {
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

			if (!this->main_contiguous_territory_rect.contains(adjacent_pos)) {
				can_expand_right = false;
				break;
			}

			const metternich::tile *adjacent_tile = map->get_tile(adjacent_pos);

			if (adjacent_tile->get_owner() != this->country) {
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

			if (!this->main_contiguous_territory_rect.contains(adjacent_pos)) {
				can_expand_up = false;
				break;
			}

			const metternich::tile *adjacent_tile = map->get_tile(adjacent_pos);

			if (adjacent_tile->get_owner() != this->country) {
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

			if (!this->main_contiguous_territory_rect.contains(adjacent_pos)) {
				can_expand_down = false;
				break;
			}

			const metternich::tile *adjacent_tile = map->get_tile(adjacent_pos);

			if (adjacent_tile->get_owner() != this->country) {
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

QVariantList country_game_data::get_resource_counts_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_resource_counts());
}

QVariantList country_game_data::get_vassal_resource_counts_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_vassal_resource_counts());
}

QVariantList country_game_data::get_tile_terrain_counts_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_tile_terrain_counts());
}

diplomacy_state country_game_data::get_diplomacy_state(const metternich::country *other_country) const
{
	const auto find_iterator = this->diplomacy_states.find(other_country);

	if (find_iterator != this->diplomacy_states.end()) {
		return find_iterator->second;
	}

	return diplomacy_state::peace;
}

void country_game_data::set_diplomacy_state(const metternich::country *other_country, const diplomacy_state state)
{
	const diplomacy_state old_state = this->get_diplomacy_state(other_country);

	if (state == old_state) {
		return;
	}

	//minor nations cannot have diplomacy with one another
	assert_throw(this->country->is_great_power() || other_country->is_great_power());

	switch (state) {
		case diplomacy_state::alliance:
			//only great powers can ally between themselves
			assert_throw(this->country->is_great_power() && other_country->is_great_power());
			break;
		case diplomacy_state::non_aggression_pact:
			//non-aggression pacts can only be formed between a great power and a minor nation
			assert_throw(this->country->is_great_power() != other_country->is_great_power());
			break;
		default:
			break;
	}

	if (is_vassalage_diplomacy_state(state)) {
		//only great powers can have vassals
		assert_throw(other_country->is_great_power());

		this->set_overlord(other_country);
	} else {
		if (this->get_overlord() == other_country) {
			this->set_overlord(nullptr);
		}
	}

	if (old_state != diplomacy_state::peace) {
		this->change_diplomacy_state_count(old_state, -1);
	}

	if (state == diplomacy_state::peace) {
		this->diplomacy_states.erase(other_country);
	} else {
		this->diplomacy_states[other_country] = state;
		this->change_diplomacy_state_count(state, 1);
	}

	if (game::get()->is_running()) {
		emit diplomacy_states_changed();

		if (is_vassalage_diplomacy_state(state) || is_vassalage_diplomacy_state(old_state)) {
			emit type_name_changed();
			emit vassalage_type_name_changed();
		}
	}
}

void country_game_data::change_diplomacy_state_count(const diplomacy_state state, const int change)
{
	const int final_count = (this->diplomacy_state_counts[state] += change);

	if (final_count == 0) {
		this->diplomacy_state_counts.erase(state);
		this->diplomacy_state_diplomatic_map_images.erase(state);
	}

	//if the change added the diplomacy state to the map, then we need to create the diplomatic map image for it
	if (game::get()->is_running() && final_count == change && !is_vassalage_diplomacy_state(state) && !is_overlordship_diplomacy_state(state)) {
		thread_pool::get()->co_spawn_sync([this, state]() -> boost::asio::awaitable<void> {
			co_await create_diplomatic_map_mode_image(diplomatic_map_mode::diplomatic, state);
		});
	}
}

QString country_game_data::get_diplomacy_state_diplomatic_map_suffix(metternich::country *other_country) const
{
	if (other_country == this->country || this->is_any_overlord_of(other_country) || this->is_any_vassal_of(other_country)) {
		return "empire";
	}

	return QString::fromStdString(enum_converter<diplomacy_state>::to_string(this->get_diplomacy_state(other_country)));
}

bool country_game_data::at_war() const
{
	return this->diplomacy_state_counts.contains(diplomacy_state::war);
}

QVariantList country_game_data::get_consulates_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->consulates);
}

void country_game_data::set_consulate(const metternich::country *other_country, const consulate *consulate)
{
	if (consulate == nullptr) {
		this->consulates.erase(other_country);
	} else {
		this->consulates[other_country] = consulate;
	}

	if (game::get()->is_running()) {
		emit consulates_changed();
	}
}

int country_game_data::get_opinion_of(const metternich::country *other) const
{
	int opinion = this->get_base_opinion(other);

	for (const auto &[modifier, duration] : this->get_opinion_modifiers_for(other)) {
		opinion += modifier->get_value();
	}

	opinion = std::clamp(opinion, character::min_opinion, character::max_opinion);

	return opinion;
}

void country_game_data::add_opinion_modifier(const metternich::country *other, const opinion_modifier *modifier, const int duration)
{
	this->opinion_modifiers[other][modifier] = std::max(this->opinion_modifiers[other][modifier], duration);
}

void country_game_data::remove_opinion_modifier(const metternich::country *other, const opinion_modifier *modifier)
{
	opinion_modifier_map<int> &opinion_modifiers = this->opinion_modifiers[other];
	opinion_modifiers.erase(modifier);

	if (opinion_modifiers.empty()) {
		this->opinion_modifiers.erase(other);
	}
}

std::vector<const metternich::country *> country_game_data::get_vassals() const
{
	std::vector<const metternich::country *> vassals;

	for (const auto &[country, diplomacy_state] : this->diplomacy_states) {
		if (is_overlordship_diplomacy_state(diplomacy_state)) {
			vassals.push_back(country);
		}
	}

	return vassals;
}

QVariantList country_game_data::get_vassals_qvariant_list() const
{
	return container::to_qvariant_list(this->get_vassals());
}

QVariantList country_game_data::get_colonies_qvariant_list() const
{
	std::vector<const metternich::country *> colonies;

	for (const auto &[country, diplomacy_state] : this->diplomacy_states) {
		if (diplomacy_state == diplomacy_state::colonial_overlord) {
			colonies.push_back(country);
		}
	}

	return container::to_qvariant_list(colonies);
}

std::vector<const metternich::country *> country_game_data::get_neighbor_countries() const
{
	std::vector<const metternich::country *> neighbor_countries;

	for (const province *province : this->get_border_provinces()) {
		for (const metternich::province *neighbor_province : province->get_game_data()->get_neighbor_provinces()) {
			const metternich::province_game_data *neighbor_province_game_data = neighbor_province->get_game_data();
			if (neighbor_province_game_data->get_owner() == this->country) {
				continue;
			}

			if (neighbor_province_game_data->get_owner() == nullptr) {
				continue;
			}

			if (vector::contains(neighbor_countries, neighbor_province_game_data->get_owner())) {
				continue;
			}

			neighbor_countries.push_back(neighbor_province_game_data->get_owner());
		}
	}

	return neighbor_countries;
}

const QColor &country_game_data::get_diplomatic_map_color() const
{
	if (this->get_overlord() != nullptr) {
		return this->get_overlord()->get_game_data()->get_diplomatic_map_color();
	}

	return this->country->get_color();
}

boost::asio::awaitable<void> country_game_data::create_diplomatic_map_image()
{
	const map *map = map::get();

	const int tile_pixel_size = map->get_diplomatic_map_tile_pixel_size();

	assert_throw(this->territory_rect.width() > 0);
	assert_throw(this->territory_rect.height() > 0);

	this->diplomatic_map_image = QImage(this->territory_rect.size(), QImage::Format_RGBA8888);
	this->diplomatic_map_image.fill(Qt::transparent);

	this->selected_diplomatic_map_image = this->diplomatic_map_image;

	const QColor &color = this->get_diplomatic_map_color();
	const QColor &selected_color = defines::get()->get_selected_country_color();

	for (int x = 0; x < this->territory_rect.width(); ++x) {
		for (int y = 0; y < this->territory_rect.height(); ++y) {
			const QPoint relative_tile_pos = QPoint(x, y);
			const tile *tile = map->get_tile(this->territory_rect.topLeft() + relative_tile_pos);

			if (tile->get_owner() != this->country) {
				continue;
			}

			this->diplomatic_map_image.setPixelColor(relative_tile_pos, color);
			this->selected_diplomatic_map_image.setPixelColor(relative_tile_pos, selected_color);
		}
	}

	QImage scaled_diplomatic_map_image;
	QImage scaled_selected_diplomatic_map_image;

	co_await thread_pool::get()->co_spawn_awaitable([this, tile_pixel_size, &scaled_diplomatic_map_image, &scaled_selected_diplomatic_map_image]() -> boost::asio::awaitable<void> {
		scaled_diplomatic_map_image = co_await image::scale<QImage::Format_ARGB32>(this->diplomatic_map_image, centesimal_int(tile_pixel_size), [](const size_t factor, const uint32_t *src, uint32_t *tgt, const int src_width, const int src_height) {
		xbrz::scale(factor, src, tgt, src_width, src_height, xbrz::ColorFormat::ARGB);
	});

	scaled_selected_diplomatic_map_image = co_await image::scale<QImage::Format_ARGB32>(this->selected_diplomatic_map_image, centesimal_int(tile_pixel_size), [](const size_t factor, const uint32_t *src, uint32_t *tgt, const int src_width, const int src_height) {
		xbrz::scale(factor, src, tgt, src_width, src_height, xbrz::ColorFormat::ARGB);
	});
	});

	this->diplomatic_map_image = std::move(scaled_diplomatic_map_image);
	this->selected_diplomatic_map_image = std::move(scaled_selected_diplomatic_map_image);

	std::vector<QPoint> border_pixels;

	for (int x = 0; x < this->diplomatic_map_image.width(); ++x) {
		for (int y = 0; y < this->diplomatic_map_image.height(); ++y) {
			const QPoint pixel_pos(x, y);
			const QColor pixel_color = this->diplomatic_map_image.pixelColor(pixel_pos);

			if (pixel_color.alpha() == 0) {
				continue;
			}

			if (pixel_pos.x() == 0 || pixel_pos.y() == 0 || pixel_pos.x() == (this->diplomatic_map_image.width() - 1) || pixel_pos.y() == (this->diplomatic_map_image.height() - 1)) {
				border_pixels.push_back(pixel_pos);
				continue;
			}

			if (pixel_color != color) {
				//blended color
				border_pixels.push_back(pixel_pos);
				continue;
			}

			bool is_border_pixel = false;
			point::for_each_cardinally_adjacent_until(pixel_pos, [this, &color, &is_border_pixel](const QPoint &adjacent_pos) {
				if (this->diplomatic_map_image.pixelColor(adjacent_pos).alpha() != 0) {
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
		this->diplomatic_map_image.setPixelColor(border_pixel_pos, border_pixel_color);
		this->selected_diplomatic_map_image.setPixelColor(border_pixel_pos, border_pixel_color);
	}

	const centesimal_int &scale_factor = preferences::get()->get_scale_factor();

	co_await thread_pool::get()->co_spawn_awaitable([this, &scale_factor, &scaled_diplomatic_map_image, &scaled_selected_diplomatic_map_image]() -> boost::asio::awaitable<void> {
		scaled_diplomatic_map_image = co_await image::scale<QImage::Format_ARGB32>(this->diplomatic_map_image, scale_factor, [](const size_t factor, const uint32_t *src, uint32_t *tgt, const int src_width, const int src_height) {
			xbrz::scale(factor, src, tgt, src_width, src_height, xbrz::ColorFormat::ARGB);
		});

		scaled_selected_diplomatic_map_image = co_await image::scale<QImage::Format_ARGB32>(this->selected_diplomatic_map_image, scale_factor, [](const size_t factor, const uint32_t *src, uint32_t *tgt, const int src_width, const int src_height) {
			xbrz::scale(factor, src, tgt, src_width, src_height, xbrz::ColorFormat::ARGB);
		});
	});

	this->diplomatic_map_image = std::move(scaled_diplomatic_map_image);
	this->selected_diplomatic_map_image = std::move(scaled_selected_diplomatic_map_image);

	this->diplomatic_map_image_rect = QRect(this->territory_rect.topLeft() * tile_pixel_size * scale_factor, this->diplomatic_map_image.size());

	co_await create_diplomatic_map_mode_image(diplomatic_map_mode::diplomatic, {});
	co_await create_diplomatic_map_mode_image(diplomatic_map_mode::diplomatic, diplomacy_state::peace);

	for (const auto &[diplomacy_state, count] : this->get_diplomacy_state_counts()) {
		if (!is_vassalage_diplomacy_state(diplomacy_state) && !is_overlordship_diplomacy_state(diplomacy_state)) {
			co_await create_diplomatic_map_mode_image(diplomatic_map_mode::diplomatic, diplomacy_state);
		}
	}

	co_await create_diplomatic_map_mode_image(diplomatic_map_mode::terrain, {});
	co_await create_diplomatic_map_mode_image(diplomatic_map_mode::cultural, {});
	co_await create_diplomatic_map_mode_image(diplomatic_map_mode::religious, {});

	emit diplomatic_map_image_changed();
}

boost::asio::awaitable<void> country_game_data::create_diplomatic_map_mode_image(const diplomatic_map_mode mode, const std::optional<diplomacy_state> &diplomacy_state)
{
	static const QColor empty_color(Qt::black);
	static constexpr QColor diplomatic_self_color(170, 148, 214);

	const map *map = map::get();

	const int tile_pixel_size = map->get_diplomatic_map_tile_pixel_size();

	assert_throw(this->territory_rect.width() > 0);
	assert_throw(this->territory_rect.height() > 0);

	QImage &image = (mode == diplomatic_map_mode::diplomatic && diplomacy_state.has_value()) ? this->diplomacy_state_diplomatic_map_images[diplomacy_state.value()] : this->diplomatic_map_mode_images[mode];

	image = QImage(this->territory_rect.size(), QImage::Format_RGBA8888);
	image.fill(Qt::transparent);

	for (int x = 0; x < this->territory_rect.width(); ++x) {
		for (int y = 0; y < this->territory_rect.height(); ++y) {
			const QPoint relative_tile_pos = QPoint(x, y);
			const tile *tile = map->get_tile(this->territory_rect.topLeft() + relative_tile_pos);

			if (tile->get_owner() != this->country) {
				continue;
			}

			const QColor *color = nullptr;

			switch (mode) {
				case diplomatic_map_mode::diplomatic:
					if (diplomacy_state.has_value()) {
						color = &defines::get()->get_diplomacy_state_color(diplomacy_state.value());
					} else {
						color = &diplomatic_self_color;
					}
					break;
				case diplomatic_map_mode::terrain:
					color = &tile->get_terrain()->get_color();
					break;
				case diplomatic_map_mode::cultural: {
					const culture *culture = tile->get_province()->get_game_data()->get_culture();
					if (culture != nullptr) {
						color = &tile->get_province()->get_game_data()->get_culture()->get_color();
					} else {
						color = &empty_color;
					}
					break;
				}
				case diplomatic_map_mode::religious: {
					const metternich::religion *religion = tile->get_province()->get_game_data()->get_religion();
					if (religion != nullptr) {
						color = &tile->get_province()->get_game_data()->get_religion()->get_color();
					} else {
						color = &empty_color;
					}
					break;
				}
			}

			image.setPixelColor(relative_tile_pos, *color);
		}
	}

	QImage scaled_image;

	co_await thread_pool::get()->co_spawn_awaitable([this, tile_pixel_size, &image, &scaled_image]() -> boost::asio::awaitable<void> {
		scaled_image = co_await image::scale<QImage::Format_ARGB32>(image, centesimal_int(tile_pixel_size), [](const size_t factor, const uint32_t *src, uint32_t *tgt, const int src_width, const int src_height) {
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
			point::for_each_cardinally_adjacent_until(pixel_pos, [this, &image, &is_border_pixel](const QPoint &adjacent_pos) {
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

	co_await thread_pool::get()->co_spawn_awaitable([this, &scale_factor, &image, &scaled_image]() -> boost::asio::awaitable<void> {
		scaled_image = co_await image::scale<QImage::Format_ARGB32>(image, scale_factor, [](const size_t factor, const uint32_t *src, uint32_t *tgt, const int src_width, const int src_height) {
			xbrz::scale(factor, src, tgt, src_width, src_height, xbrz::ColorFormat::ARGB);
		});
	});

	image = std::move(scaled_image);
}

void country_game_data::change_score(const int change)
{
	if (change == 0) {
		return;
	}

	const int old_vassal_score = this->get_score() * country::vassal_score_percent / 100;

	this->score += change;

	const int new_vassal_score = this->get_score() * country::vassal_score_percent / 100;

	if (this->get_overlord() != nullptr) {
		const int vassal_score_change = new_vassal_score - old_vassal_score;
		this->get_overlord()->get_game_data()->change_score(vassal_score_change);
	}

	emit score_changed();
}

void country_game_data::add_population_unit(qunique_ptr<population_unit> &&population_unit)
{
	this->change_population_type_count(population_unit->get_type(), 1);
	this->change_population_culture_count(population_unit->get_culture(), 1);
	this->change_population_religion_count(population_unit->get_religion(), 1);
	this->change_population_phenotype_count(population_unit->get_phenotype(), 1);
	if (population_unit->get_ideology() != nullptr) {
		this->change_population_ideology_count(population_unit->get_ideology(), 1);
	}
	this->change_population(defines::get()->get_population_per_unit());

	this->population_units.push_back(std::move(population_unit));

	if (game::get()->is_running()) {
		emit population_units_changed();
	}
}

qunique_ptr<population_unit> country_game_data::pop_population_unit(population_unit *population_unit)
{
	for (size_t i = 0; i < this->population_units.size();) {
		if (this->population_units[i].get() == population_unit) {
			qunique_ptr<metternich::population_unit> population_unit_unique_ptr = std::move(this->population_units[i]);
			this->population_units.erase(this->population_units.begin() + i);

			this->change_population_type_count(population_unit->get_type(), -1);
			this->change_population_culture_count(population_unit->get_culture(), -1);
			this->change_population_religion_count(population_unit->get_religion(), -1);
			this->change_population_phenotype_count(population_unit->get_phenotype(), -1);
			if (population_unit->get_ideology() != nullptr) {
				this->change_population_ideology_count(population_unit->get_ideology(), -1);
			}
			this->change_population(-defines::get()->get_population_per_unit());

			if (game::get()->is_running()) {
				emit population_units_changed();
			}

			return population_unit_unique_ptr;
		} else {
			++i;
		}
	}

	assert_throw(false);

	return nullptr;
}

void country_game_data::create_population_unit(const population_type *type, const metternich::culture *culture, const metternich::religion *religion, const phenotype *phenotype)
{
	auto population_unit = make_qunique<metternich::population_unit>(type, culture, religion, phenotype, this->country);
	this->add_population_unit(std::move(population_unit));
}

QVariantList country_game_data::get_population_type_counts_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_population_type_counts());
}

void country_game_data::change_population_type_count(const population_type *type, const int change)
{
	if (change == 0) {
		return;
	}

	const int count = (this->population_type_counts[type] += change);

	assert_throw(count >= 0);

	if (count == 0) {
		this->population_type_counts.erase(type);
	}

	if (game::get()->is_running()) {
		emit population_type_counts_changed();
	}

	if (type->get_output_commodity() != nullptr) {
		this->change_commodity_output(type->get_output_commodity(), type->get_output_value() * change);
	}
}

QVariantList country_game_data::get_population_culture_counts_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_population_culture_counts());
}

void country_game_data::change_population_culture_count(const culture *culture, const int change)
{
	if (change == 0) {
		return;
	}

	const int count = (this->population_culture_counts[culture] += change);

	assert_throw(count >= 0);

	if (count == 0) {
		this->population_culture_counts.erase(culture);
	}

	if (game::get()->is_running()) {
		emit population_culture_counts_changed();
	}
}

QVariantList country_game_data::get_population_religion_counts_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_population_religion_counts());
}

void country_game_data::change_population_religion_count(const metternich::religion *religion, const int change)
{
	if (change == 0) {
		return;
	}

	const int count = (this->population_religion_counts[religion] += change);

	assert_throw(count >= 0);

	if (count == 0) {
		this->population_religion_counts.erase(religion);
	}

	if (game::get()->is_running()) {
		emit population_religion_counts_changed();
	}
}

QVariantList country_game_data::get_population_phenotype_counts_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_population_phenotype_counts());
}

void country_game_data::change_population_phenotype_count(const phenotype *phenotype, const int change)
{
	if (change == 0) {
		return;
	}

	const int count = (this->population_phenotype_counts[phenotype] += change);

	assert_throw(count >= 0);

	if (count == 0) {
		this->population_phenotype_counts.erase(phenotype);
	}

	if (game::get()->is_running()) {
		emit population_phenotype_counts_changed();
	}
}

QVariantList country_game_data::get_population_ideology_counts_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_population_ideology_counts());
}

void country_game_data::change_population_ideology_count(const ideology *ideology, const int change)
{
	if (change == 0) {
		return;
	}

	const int count = (this->population_ideology_counts[ideology] += change);

	assert_throw(count >= 0);

	if (count == 0) {
		this->population_ideology_counts.erase(ideology);
	}

	if (game::get()->is_running()) {
		emit population_ideology_counts_changed();
	}
}

void country_game_data::change_population(const int change)
{
	if (change == 0) {
		return;
	}

	this->population += change;

	if (game::get()->is_running()) {
		emit population_changed();
	}
}

void country_game_data::set_population_growth(const int growth)
{
	if (growth == this->get_population_growth()) {
		return;
	}

	const int change = growth - this->get_population_growth();

	this->population_growth = growth;

	this->change_population(change * defines::get()->get_population_per_unit() / defines::get()->get_population_growth_threshold());

	if (game::get()->is_running()) {
		emit population_growth_changed();
	}
}

void country_game_data::grow_population()
{
	if (this->population_units.empty()) {
		throw std::runtime_error("Tried to grow population in a province which has no pre-existing population.");
	}

	const qunique_ptr<population_unit> &population_unit = vector::get_random(this->population_units);
	const metternich::culture *culture = population_unit->get_culture();
	const metternich::religion *religion = population_unit->get_religion();
	const phenotype *phenotype = population_unit->get_phenotype();
	const population_type *population_type = culture->get_population_class_type(defines::get()->get_default_population_class());

	this->create_population_unit(population_type, culture, religion, phenotype);

	this->change_population_growth(-defines::get()->get_population_growth_threshold());
}

void country_game_data::decrease_population()
{
	//disband population unit, if possible
	if (!this->population_units.empty()) {
		this->change_population_growth(1);
		this->pop_population_unit(this->choose_starvation_population_unit());
		return;
	}

	//disband civilian unit, if possible
	civilian_unit *best_civilian_unit = nullptr;

	for (auto it = this->civilian_units.rbegin(); it != this->civilian_units.rend(); ++it) {
		civilian_unit *civilian_unit = it->get();

		if (
			best_civilian_unit == nullptr
			|| (best_civilian_unit->is_busy() && !civilian_unit->is_busy())
		) {
			best_civilian_unit = civilian_unit;
		}
	}

	if (best_civilian_unit != nullptr) {
		best_civilian_unit->disband(false);
		this->change_population_growth(1);
		return;
	}

	//disband military unit, if possible
	for (auto it = this->military_units.rbegin(); it != this->military_units.rend(); ++it) {
		military_unit *military_unit = it->get();

		if (military_unit->get_character() != nullptr) {
			//character military units do not cost food, so disbanding them does nothing to help with starvation
			continue;
		}

		military_unit->disband(false);
		this->change_population_growth(1);
		return;
	}

	assert_throw(false);
}

population_unit *country_game_data::choose_starvation_population_unit()
{
	population_unit *best_population_unit = nullptr;

	for (auto it = this->population_units.rbegin(); it != this->population_units.rend(); ++it) {
		population_unit *population_unit = it->get();

		if (
			best_population_unit == nullptr
			|| best_population_unit->get_type()->get_output_value() < population_unit->get_type()->get_output_value()
		) {
			best_population_unit = population_unit;
		}
	}

	assert_throw(best_population_unit != nullptr);
	return best_population_unit;
}

QObject *country_game_data::get_population_type_small_icon(population_type *type) const
{
	icon_map<int> icon_counts;

	for (const auto &population_unit : this->population_units) {
		if (population_unit->get_type() != type) {
			continue;
		}

		++icon_counts[population_unit->get_small_icon()];
	}

	const icon *best_icon = nullptr;
	int best_icon_count = 0;
	for (const auto &[icon, count] : icon_counts) {
		if (count > best_icon_count) {
			best_icon = icon;
			best_icon_count = count;
		}
	}

	return const_cast<icon *>(best_icon);
}

QVariantList country_game_data::get_building_slots_qvariant_list() const
{
	std::vector<const building_slot *> available_building_slots;

	for (const qunique_ptr<building_slot> &building_slot : this->building_slots) {
		if (!building_slot->is_available()) {
			continue;
		}

		available_building_slots.push_back(building_slot.get());
	}

	return container::to_qvariant_list(available_building_slots);
}

void country_game_data::initialize_building_slots()
{
	//initialize building slots, placing them in random order
	std::vector<building_slot_type *> building_slot_types = building_slot_type::get_all();
	vector::shuffle(building_slot_types);

	for (const building_slot_type *building_slot_type : building_slot_types) {
		this->building_slots.push_back(make_qunique<building_slot>(building_slot_type, this->country));
		this->building_slot_map[building_slot_type] = this->building_slots.back().get();
	}
}

void country_game_data::initialize_free_buildings()
{
	assert_throw(this->country->get_culture() != nullptr);

	//add a free warehouse
	for (const qunique_ptr<building_slot> &building_slot : this->building_slots) {
		const building_type *buildable_warehouse = nullptr;

		for (const building_type *building_type : building_slot->get_type()->get_building_types()) {
			if (!building_type->is_warehouse()) {
				continue;
			}

			if (building_type != this->country->get_culture()->get_building_class_type(building_type->get_building_class())) {
				continue;
			}

			if (building_type->get_required_technology() != nullptr) {
				//only a basic warehouse is free
				continue;
			}

			if (!building_slot->can_have_building(building_type)) {
				continue;
			}

			buildable_warehouse = building_type;
			break;
		}

		if (buildable_warehouse == nullptr) {
			continue;
		}

		building_slot->set_building(buildable_warehouse);
		break;
	}
}

const building_type *country_game_data::get_slot_building(const building_slot_type *slot_type) const
{
	const auto find_iterator = this->building_slot_map.find(slot_type);
	if (find_iterator != this->building_slot_map.end()) {
		return find_iterator->second->get_building();
	}

	assert_throw(false);

	return nullptr;
}

void country_game_data::set_slot_building(const building_slot_type *slot_type, const building_type *building)
{
	if (building != nullptr) {
		assert_throw(building->get_building_class()->get_slot_type() == slot_type);
	}

	const auto find_iterator = this->building_slot_map.find(slot_type);
	if (find_iterator != this->building_slot_map.end()) {
		find_iterator->second->set_building(building);
		return;
	}

	assert_throw(false);
}

void country_game_data::clear_buildings()
{
	for (const qunique_ptr<building_slot> &building_slot : this->building_slots) {
		building_slot->set_building(nullptr);
	}
}

void country_game_data::on_building_gained(const building_type *building, const int multiplier)
{
	assert_throw(building != nullptr);

	this->change_score(building->get_score() * multiplier);
}

QVariantList country_game_data::get_stored_commodities_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_stored_commodities());
}

int country_game_data::get_stored_commodity(const QString &commodity_identifier) const
{
	return this->get_stored_commodity(commodity::get(commodity_identifier.toStdString()));
}

void country_game_data::set_stored_commodity(const commodity *commodity, const int value)
{
	if (value == this->get_stored_commodity(commodity)) {
		return;
	}

	if (value < 0) {
		throw std::runtime_error("Tried to set the storage of commodity \"" + commodity->get_identifier() + "\" for country \"" + this->country->get_identifier() + "\" to a negative number.");
	}

	if (commodity->is_convertible_to_wealth()) {
		assert_throw(value > 0);
		this->change_wealth(commodity->get_wealth_value() * value);
		return;
	}

	if (value > this->get_storage_capacity() && !commodity->is_abstract()) {
		this->set_stored_commodity(commodity, this->get_storage_capacity());
		return;
	}

	if (value <= 0) {
		this->stored_commodities.erase(commodity);
	} else {
		this->stored_commodities[commodity] = value;
	}

	if (game::get()->is_running()) {
		emit stored_commodities_changed();
	}
}

void country_game_data::set_storage_capacity(const int capacity)
{
	if (capacity == this->get_storage_capacity()) {
		return;
	}

	this->storage_capacity = capacity;

	if (game::get()->is_running()) {
		emit storage_capacity_changed();
	}
}

QVariantList country_game_data::get_commodity_inputs_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_commodity_inputs());
}

int country_game_data::get_commodity_input(const QString &commodity_identifier) const
{
	return this->get_commodity_input(commodity::get(commodity_identifier.toStdString()));
}

void country_game_data::change_commodity_input(const commodity *commodity, const int change)
{
	if (change == 0) {
		return;
	}

	const int count = (this->commodity_inputs[commodity] += change);

	assert_throw(count >= 0);

	if (count == 0) {
		this->commodity_inputs.erase(commodity);
	}

	if (game::get()->is_running()) {
		emit commodity_inputs_changed();
	}
}

QVariantList country_game_data::get_commodity_outputs_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_commodity_outputs());
}

int country_game_data::get_commodity_output(const QString &commodity_identifier) const
{
	return this->get_commodity_output(commodity::get(commodity_identifier.toStdString()));
}

void country_game_data::change_commodity_output(const commodity *commodity, const int change)
{
	if (change == 0) {
		return;
	}

	const int count = (this->commodity_outputs[commodity] += change);

	assert_throw(count >= 0);

	if (count == 0) {
		this->commodity_outputs.erase(commodity);
	}

	if (game::get()->is_running()) {
		emit commodity_outputs_changed();
	}

	if (change < 0 && !commodity->is_storable()) {
		//decrease consumption of non-storable commodities immediately if the net output goes below zero, since for those commodities consumption cannot be fulfilled by storage
		while (this->get_net_commodity_output(commodity) < 0) {
			this->decrease_commodity_consumption(commodity);
		}
	}
}

void country_game_data::assign_production()
{
	bool changed = true;

	while (changed) {
		changed = false;

		for (const qunique_ptr<building_slot> &building_slot : this->building_slots) {
			const building_type *building_type = building_slot->get_building();

			if (building_type == nullptr) {
				continue;
			}

			for (const production_type *production_type : building_slot->get_available_production_types()) {
				if (!building_slot->can_increase_production(production_type)) {
					continue;
				}

				building_slot->increase_production(production_type);
				changed = true;
			}
		}
	}
}

void country_game_data::decrease_commodity_consumption(const commodity *commodity, const bool restore_inputs)
{
	for (const qunique_ptr<building_slot> &building_slot : this->building_slots) {
		const building_type *building_type = building_slot->get_building();

		if (building_type == nullptr) {
			continue;
		}

		for (const production_type *production_type : building_slot->get_available_production_types()) {
			if (!production_type->get_input_commodities().contains(commodity)) {
				continue;
			}

			if (!building_slot->can_decrease_production(production_type)) {
				continue;
			}

			building_slot->decrease_production(production_type, restore_inputs);
			return;
		}
	}

	assert_throw(false);
}

bool country_game_data::produces_commodity(const commodity *commodity) const
{
	if (this->get_commodity_output(commodity) > 0) {
		return true;
	}

	for (const province *province : this->get_provinces()) {
		if (province->get_game_data()->produces_commodity(commodity)) {
			return true;
		}
	}

	for (const qunique_ptr<building_slot> &building_slot : this->building_slots) {
		for (const production_type *production_type : building_slot->get_available_production_types()) {
			if (production_type->get_output_commodity() == commodity && building_slot->get_production_type_output(production_type) > 0) {
				return true;
			}
		}
	}

	return false;
}

bool country_game_data::can_declare_war_on(const metternich::country *other_country) const
{
	if (!this->country->can_declare_war()) {
		return false;
	}

	if (this->get_overlord() != nullptr) {
		return other_country == this->get_overlord();
	}

	return true;
}

QVariantList country_game_data::get_technologies_qvariant_list() const
{
	return container::to_qvariant_list(this->technologies);
}

void country_game_data::add_technology(const technology *technology)
{
	if (this->has_technology(technology)) {
		return;
	}

	this->technologies.insert(technology);

	if (technology->get_modifier() != nullptr) {
		technology->get_modifier()->apply(this->country, 1);
	}

	if (game::get()->is_running()) {
		emit technologies_changed();
	}
}

void country_game_data::add_technology_with_prerequisites(const technology *technology)
{
	this->add_technology(technology);

	for (const metternich::technology *prerequisite : technology->get_prerequisites()) {
		this->add_technology_with_prerequisites(prerequisite);
	}
}

std::vector<const technology *> country_game_data::get_available_technologies() const
{
	std::vector<const technology *> available_technologies;

	for (const technology *technology : technology::get_all()) {
		if (technology->is_discovery()) {
			continue;
		}

		if (this->has_technology(technology)) {
			continue;
		}

		bool has_prerequisites = true;
		for (const metternich::technology *prerequisite : technology->get_prerequisites()) {
			if (!this->has_technology(prerequisite)) {
				has_prerequisites = false;
				break;
			}
		}

		if (!has_prerequisites) {
			continue;
		}

		available_technologies.push_back(technology);
	}

	std::sort(available_technologies.begin(), available_technologies.end(), technology_compare());

	return available_technologies;
}

QVariantList country_game_data::get_available_technologies_qvariant_list() const
{
	return container::to_qvariant_list(this->get_available_technologies());
}

QVariantList country_game_data::get_future_technologies_qvariant_list() const
{
	std::vector<technology *> future_technologies = technology::get_all();
	std::erase_if(future_technologies, [this](const technology *technology) {
		if (this->has_technology(technology)) {
			return true;
		}

		bool has_all_prerequisites = true;
		for (const metternich::technology *prerequisite : technology->get_prerequisites()) {
			if (!this->has_technology(prerequisite)) {
				has_all_prerequisites = false;
			}
		}
		if (has_all_prerequisites) {
			return true;
		}

		return false;
	});

	std::sort(future_technologies.begin(), future_technologies.end(), technology_compare());

	return container::to_qvariant_list(future_technologies);
}

void country_game_data::set_current_research(const technology *technology)
{
	if (technology == this->get_current_research()) {
		return;
	}

	this->current_research = technology;

	this->set_stored_commodity(defines::get()->get_research_commodity(), 0);

	emit current_research_changed();
}

void country_game_data::choose_current_research()
{
	const std::vector<const technology *> potential_technologies = this->get_available_technologies();

	if (potential_technologies.empty()) {
		return;
	}

	const technology *chosen_technology = vector::get_random(potential_technologies);
	this->set_current_research(chosen_technology);
}

QVariantList country_game_data::get_advisors_qvariant_list() const
{
	return container::to_qvariant_list(this->get_advisors());
}

void country_game_data::check_advisors()
{
	//remove obsolete advisors
	const std::vector<const character *> advisors = this->get_advisors();
	for (const character *advisor : advisors) {
		if (advisor->get_obsolescence_technology() != nullptr && this->has_technology(advisor->get_obsolescence_technology())) {
			if (this->country == game::get()->get_player_country()) {
				const icon *interior_minister_portrait = defines::get()->get_interior_minister_portrait();

				engine_interface::get()->add_notification("Advisor Retired", interior_minister_portrait, std::format("Your Excellency, after a distinguished career in our service, the advisor {} has decided to retire.", this->get_next_advisor()->get_full_name()));
			}

			this->remove_advisor(advisor);
		}
	}

	if (this->get_next_advisor() != nullptr) {
		if (this->get_next_advisor()->get_game_data()->get_country() != nullptr) {
			if (this->country == game::get()->get_player_country()) {
				const icon *interior_minister_portrait = defines::get()->get_interior_minister_portrait();

				engine_interface::get()->add_notification("Advisor Unavailable", interior_minister_portrait, std::format("Your Excellency, the advisor {} has unfortunately decided to join {}, and is no longer available for recruitment.", this->get_next_advisor()->get_full_name(), this->get_next_advisor()->get_game_data()->get_country()->get_name()));
			}

			this->set_next_advisor(nullptr);
		} else if (this->get_next_advisor()->get_obsolescence_technology() != nullptr && this->has_technology(this->get_next_advisor()->get_obsolescence_technology())) {
			if (this->country == game::get()->get_player_country()) {
				const icon *interior_minister_portrait = defines::get()->get_interior_minister_portrait();

				engine_interface::get()->add_notification("Advisor Unavailable", interior_minister_portrait, std::format("Your Excellency, the advisor {} is no longer available for recruitment.", this->get_next_advisor()->get_full_name()));
			}

			this->set_next_advisor(nullptr);
		} else {
			if (this->get_stored_commodity(defines::get()->get_advisor_commodity()) >= this->get_advisor_cost()) {
				this->add_advisor(this->get_next_advisor());

				if (this->country == game::get()->get_player_country()) {
					const icon *interior_minister_portrait = defines::get()->get_interior_minister_portrait();

					engine_interface::get()->add_notification("Advisor Recruited", interior_minister_portrait, std::format("Your Excellency, {} has joined our nation as an advisor!", this->get_next_advisor()->get_full_name()));
				}

				this->set_next_advisor(nullptr);
			}
		}
	} else {
		this->choose_next_advisor();
	}
}

void country_game_data::add_advisor(const character *advisor)
{
	assert_throw(game::get()->get_rules()->are_advisors_enabled());
	assert_throw(this->can_have_advisors());

	this->advisors.push_back(advisor);
	advisor->get_game_data()->set_country(this->country);
	advisor->apply_advisor_modifier(this->country, 1);

	emit advisors_changed();
}

void country_game_data::remove_advisor(const character *advisor)
{
	assert_throw(advisor->get_game_data()->get_country() == this->country);

	std::erase(this->advisors, advisor);
	advisor->get_game_data()->set_country(nullptr);
	advisor->apply_advisor_modifier(this->country, -1);

	emit advisors_changed();
}

void country_game_data::clear_advisors()
{
	const std::vector<const character *> advisors = this->get_advisors();
	for (const character *advisor : advisors) {
		advisor->get_game_data()->set_country(nullptr);
	}

	assert_throw(this->get_advisors().empty());

	emit advisors_changed();
}

void country_game_data::choose_next_advisor()
{
	std::map<advisor_category, std::vector<const character *>> potential_advisors_per_category;

	for (const character *character : character::get_all()) {
		if (!character->is_advisor()) {
			continue;
		}

		const character_game_data *character_game_data = character->get_game_data();
		if (character_game_data->get_country() != nullptr) {
			continue;
		}

		if (character->get_required_technology() != nullptr && !this->has_technology(character->get_required_technology())) {
			continue;
		}

		if (character->get_obsolescence_technology() != nullptr && this->has_technology(character->get_obsolescence_technology())) {
			continue;
		}

		if (character->get_conditions() != nullptr && !character->get_conditions()->check(this->country, read_only_context(this->country))) {
			continue;
		}

		int weight = 1;

		if (character->get_home_province()->get_game_data()->get_owner() == this->country) {
			weight += 4;
		}

		std::vector<const metternich::character *> &category_advisors = potential_advisors_per_category[character->get_advisor_type()->get_category()];

		for (int i = 0; i < weight; ++i) {
			category_advisors.push_back(character);
		}
	}

	if (potential_advisors_per_category.empty()) {
		return;
	}

	std::map<advisor_category, const character *> potential_advisor_map;
	const std::vector<advisor_category> potential_categories = archimedes::map::get_keys(potential_advisors_per_category);

	for (const advisor_category category : potential_categories) {
		potential_advisor_map[category] = vector::get_random(potential_advisors_per_category[category]);
	}


	if (this->is_ai()) {
		const advisor_category chosen_category = vector::get_random(potential_categories);
		this->set_next_advisor(potential_advisor_map[chosen_category]);
	} else {
		const std::vector<const character *> potential_advisors = archimedes::map::get_values(potential_advisor_map);
		emit engine_interface::get()->next_advisor_choosable(container::to_qvariant_list(potential_advisors));
	}
}

bool country_game_data::can_have_advisors() const
{
	//only great powers can have advisors
	return this->country->get_type() == country_type::great_power;
}

void country_game_data::add_civilian_unit(qunique_ptr<civilian_unit> &&civilian_unit)
{
	this->civilian_units.push_back(std::move(civilian_unit));
}

void country_game_data::remove_civilian_unit(civilian_unit *civilian_unit)
{
	for (size_t i = 0; i < this->civilian_units.size(); ++i) {
		if (this->civilian_units[i].get() == civilian_unit) {
			this->civilian_units.erase(this->civilian_units.begin() + i);
			return;
		}
	}
}

void country_game_data::add_military_unit(qunique_ptr<military_unit> &&military_unit)
{
	this->military_units.push_back(std::move(military_unit));
}

void country_game_data::remove_military_unit(military_unit *military_unit)
{
	for (size_t i = 0; i < this->military_units.size(); ++i) {
		if (this->military_units[i].get() == military_unit) {
			this->military_units.erase(this->military_units.begin() + i);
			return;
		}
	}
}

const military_unit_type *country_game_data::get_best_military_unit_category_type(const military_unit_category category) const
{
	const military_unit_type *best_type = nullptr;
	int best_score = 0;

	for (const military_unit_class *military_unit_class : military_unit_class::get_all()) {
		if (military_unit_class->get_category() != category) {
			continue;
		}

		const military_unit_type *type = this->country->get_culture()->get_military_class_unit_type(military_unit_class);

		if (type == nullptr) {
			continue;
		}

		if (type->get_required_technology() != nullptr && !this->has_technology(type->get_required_technology())) {
			continue;
		}

		const int score = type->get_score();

		if (score > best_score) {
			best_type = type;
		}
	}

	return best_type;
}

void country_game_data::set_output_modifier(const int value)
{
	if (value == this->get_output_modifier()) {
		return;
	}

	for (const qunique_ptr<building_slot> &building_slot : this->building_slots) {
		for (const production_type *production_type : building_slot->get_available_production_types()) {
			this->change_commodity_output(production_type->get_output_commodity(), -building_slot->get_production_type_output(production_type));
		}
	}

	this->output_modifier = value;

	for (const qunique_ptr<building_slot> &building_slot : this->building_slots) {
		for (const production_type *production_type : building_slot->get_available_production_types()) {
			this->change_commodity_output(production_type->get_output_commodity(), building_slot->get_production_type_output(production_type));
		}
	}

	if (game::get()->is_running()) {
		emit output_modifier_changed();
	}
}

void country_game_data::set_commodity_output_modifier(const commodity *commodity, const int value)
{
	if (value == this->get_commodity_output_modifier(commodity)) {
		return;
	}

	for (const qunique_ptr<building_slot> &building_slot : this->building_slots) {
		for (const production_type *production_type : building_slot->get_available_production_types()) {
			if (production_type->get_output_commodity() != commodity) {
				continue;
			}

			this->change_commodity_output(production_type->get_output_commodity(), -building_slot->get_production_type_output(production_type));
		}
	}

	if (value == 0) {
		this->commodity_output_modifiers.erase(commodity);
	} else {
		this->commodity_output_modifiers[commodity] = value;
	}

	for (const qunique_ptr<building_slot> &building_slot : this->building_slots) {
		for (const production_type *production_type : building_slot->get_available_production_types()) {
			if (production_type->get_output_commodity() != commodity) {
				continue;
			}

			this->change_commodity_output(production_type->get_output_commodity(), building_slot->get_production_type_output(production_type));
		}
	}
}

void country_game_data::set_category_research_modifier(const technology_category category, const int value)
{
	if (value == this->get_category_research_modifier(category)) {
		return;
	}

	if (value == 0) {
		this->category_research_modifiers.erase(category);
	} else {
		this->category_research_modifiers[category] = value;
	}
}

void country_game_data::decrement_scripted_modifiers()
{
	//decrement opinion modifiers
	country_map<std::vector<const opinion_modifier *>> opinion_modifiers_to_remove;

	for (auto &[country, opinion_modifier_map] : this->opinion_modifiers) {
		for (auto &[modifier, duration] : opinion_modifier_map) {
			if (duration == -1) {
				//eternal
				continue;
			}

			--duration;

			if (duration == 0) {
				opinion_modifiers_to_remove[country].push_back(modifier);
			}
		}
	}

	for (const auto &[country, opinion_modifiers] : opinion_modifiers_to_remove) {
		for (const opinion_modifier *modifier : opinion_modifiers) {
			this->remove_opinion_modifier(country, modifier);
		}
	}
}

}
