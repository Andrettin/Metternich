#include "metternich.h"

#include "country/country_game_data.h"

#include "character/character.h"
#include "character/character_game_data.h"
#include "character/office.h"
#include "character/office_type.h"
#include "country/country.h"
#include "country/country_type.h"
#include "country/culture.h"
#include "country/diplomacy_state.h"
#include "country/landed_title.h"
#include "country/landed_title_game_data.h"
#include "country/religion.h"
#include "database/defines.h"
#include "database/preferences.h"
#include "economy/commodity.h"
#include "economy/resource.h"
#include "game/country_event.h"
#include "game/event_trigger.h"
#include "game/game.h"
#include "map/diplomatic_map_mode.h"
#include "map/map.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "population/phenotype.h"
#include "population/population_unit.h"
#include "script/condition/condition.h"
#include "script/factor.h"
#include "technology/technology.h"
#include "unit/civilian_unit.h"
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
}

country_game_data::~country_game_data()
{
}

void country_game_data::do_turn()
{
	if (this->get_quarterly_prestige() != 0) {
		this->change_prestige(this->get_quarterly_prestige());
	}

	if (this->get_quarterly_piety() != 0) {
		this->change_piety(this->get_quarterly_piety());
	}

	this->do_migration();

	for (const province *province : this->get_provinces()) {
		province->get_game_data()->do_turn();
	}

	this->do_population_growth();

	for (const qunique_ptr<civilian_unit> &civilian_unit : this->civilian_units) {
		civilian_unit->do_turn();
	}

	this->do_events();

	for (const character *character : this->get_characters()) {
		character->get_game_data()->do_turn();
	}

	this->check_characters(game::get()->get_next_date());
}

void country_game_data::do_population_growth()
{
	if (this->population_units.empty()) {
		this->set_population_growth(0);
		return;
	}

	int stored_food = 0;
	for (const auto &[commodity, quantity] : this->get_stored_commodities()) {
		if (commodity->is_food()) {
			stored_food += quantity;
		}
	}

	int food_consumption = 0;

	for (const province *province : this->get_provinces()) {
		food_consumption += province->get_game_data()->get_food_consumption() - province->get_game_data()->get_free_food_consumption();
	}

	const int net_food = stored_food - food_consumption;

	this->change_population_growth(net_food);

	for (auto &[commodity, quantity] : this->stored_commodities) {
		if (!commodity->is_food()) {
			continue;
		}

		quantity = 0;
	}

	while (this->get_population_growth() >= defines::get()->get_population_growth_threshold()) {
		this->get_random_population_weighted_province()->get_game_data()->grow_population();
	}

	while (this->get_population_growth() <= -defines::get()->get_population_growth_threshold()) {
		//starvation
		this->decrease_population();

		if (this->population_units.empty()) {
			this->set_population_growth(0);
			return;
		}
	}
}

void country_game_data::do_migration()
{
	const std::vector<population_unit *> population_units = this->population_units;

	for (population_unit *population_unit : population_units) {
		if (population_unit->is_employed()) {
			continue;
		}

		for (const province *province : this->get_provinces()) {
			province_game_data *province_game_data = province->get_game_data();

			if (province_game_data->has_employment_for_worker(population_unit)) {
				population_unit->migrate_to(province);
				break;
			}
		}
	}
}

void country_game_data::do_events()
{
	const bool is_last_turn_of_year = (game::get()->get_date().date().month() + defines::get()->get_months_per_turn()) > 12;

	if (is_last_turn_of_year) {
		country_event::check_events_for_scope(this->country, event_trigger::yearly_pulse);
	}

	country_event::check_events_for_scope(this->country, event_trigger::quarterly_pulse);
}

void country_game_data::do_ai_turn()
{
	for (const qunique_ptr<civilian_unit> &civilian_unit : this->civilian_units) {
		civilian_unit->do_ai_turn();
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
	this->change_population(province_game_data->get_population());

	if (province_game_data->is_coastal()) {
		++this->coastal_province_count;
	}

	for (const auto &[population_type, count] : province_game_data->get_population_type_counts()) {
		this->change_population_type_count(population_type, count);
	}
	for (const auto &[culture, count] : province_game_data->get_population_culture_counts()) {
		this->change_population_culture_count(culture, count);
	}
	for (const auto &[religion, count] : province_game_data->get_population_religion_counts()) {
		this->change_population_religion_count(religion, count);
	}
	for (const auto &[phenotype, count] : province_game_data->get_population_phenotype_counts()) {
		this->change_population_phenotype_count(phenotype, count);
	}
	for (const auto &[ideology, count] : province_game_data->get_population_ideology_counts()) {
		this->change_population_ideology_count(ideology, count);
	}

	for (const qunique_ptr<population_unit> &population_unit : province_game_data->get_population_units()) {
		this->add_population_unit(population_unit.get());
	}

	for (const auto &[resource, count] : province_game_data->get_resource_counts()) {
		this->change_resource_count(resource, count);

		if (this->get_overlord() != nullptr) {
			this->get_overlord()->get_game_data()->change_vassal_resource_count(resource, count);
		}
	}

	for (const auto &[terrain, count] : province_game_data->get_tile_terrain_counts()) {
		this->change_tile_terrain_count(terrain, count);
	}

	for (const metternich::province *border_province : province_game_data->get_border_provinces()) {
		const metternich::province_game_data *border_province_game_data = border_province->get_game_data();
		if (border_province_game_data->get_owner() != this->country) {
			continue;
		}

		for (const QPoint &tile_pos : border_province_game_data->get_border_tiles()) {
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
	this->change_population(-province_game_data->get_population());

	if (province_game_data->is_coastal()) {
		--this->coastal_province_count;
	}

	for (const auto &[population_type, count] : province_game_data->get_population_type_counts()) {
		this->change_population_type_count(population_type, -count);
	}
	for (const auto &[culture, count] : province_game_data->get_population_culture_counts()) {
		this->change_population_culture_count(culture, -count);
	}
	for (const auto &[religion, count] : province_game_data->get_population_religion_counts()) {
		this->change_population_religion_count(religion, -count);
	}
	for (const auto &[phenotype, count] : province_game_data->get_population_phenotype_counts()) {
		this->change_population_phenotype_count(phenotype, -count);
	}
	for (const auto &[ideology, count] : province_game_data->get_population_ideology_counts()) {
		this->change_population_ideology_count(ideology, -count);
	}

	for (const qunique_ptr<population_unit> &population_unit : province_game_data->get_population_units()) {
		this->remove_population_unit(population_unit.get());
	}

	for (const auto &[resource, count] : province_game_data->get_resource_counts()) {
		this->change_resource_count(resource, -count);

		if (this->get_overlord() != nullptr) {
			this->get_overlord()->get_game_data()->change_vassal_resource_count(resource, -count);
		}
	}

	for (const auto &[terrain, count] : province_game_data->get_tile_terrain_counts()) {
		this->change_tile_terrain_count(terrain, -count);
	}

	for (const QPoint &tile_pos : province_game_data->get_border_tiles()) {
		std::erase(this->border_tiles, tile_pos);
	}

	for (const metternich::province *border_province : province_game_data->get_border_provinces()) {
		const metternich::province_game_data *border_province_game_data = border_province->get_game_data();
		if (border_province_game_data->get_owner() != this->country) {
			continue;
		}

		for (const QPoint &tile_pos : border_province_game_data->get_border_tiles()) {
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

const province *country_game_data::get_random_population_weighted_province() const
{
	if (this->population_units.empty()) {
		return nullptr;
	}

	return vector::get_random(this->population_units)->get_province();
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

void country_game_data::decrease_population()
{
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
		return;
	}

	this->get_random_population_weighted_province()->get_game_data()->decrease_population();
}

QVariantList country_game_data::get_stored_commodities_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_stored_commodities());
}

void country_game_data::set_stored_commodity(const commodity *commodity, const int value)
{
	if (value == this->get_stored_commodity(commodity)) {
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
	this->technologies.insert(technology);

	if (game::get()->is_running()) {
		emit technologies_changed();
	}
}

QVariantList country_game_data::get_available_technologies_qvariant_list() const
{
	std::vector<technology *> available_technologies = technology::get_all();
	std::erase_if(available_technologies, [this](const technology *technology) {
		if (this->has_technology(technology)) {
			return true;
		}

		for (const metternich::technology *prerequisite : technology->get_prerequisites()) {
			if (!this->has_technology(prerequisite)) {
				return true;
			}
		}

		return false;
	});

	std::sort(available_technologies.begin(), available_technologies.end(), technology_compare());

	return container::to_qvariant_list(available_technologies);
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

QVariantList country_game_data::get_characters_qvariant_list() const
{
	return container::to_qvariant_list(this->get_characters());
}

void country_game_data::check_characters(const QDateTime &date)
{
	const std::vector<const character *> characters = this->get_characters();
	for (const character *character : characters) {
		const metternich::country *home_province_owner = character->get_home_province()->get_game_data()->get_owner();

		if (game::get()->get_date() >= character->get_end_date()) {
			if (character->get_game_data()->is_ruler()) {
				read_only_context ctx = read_only_context::from_scope(this->country);
				ctx.current_character = character;
				country_event::check_events_for_scope(this->country, event_trigger::ruler_death, ctx);
			}

			this->remove_character(character);
		} else if (home_province_owner != this->country) {
			//if we lost their home province, move the character to the province's new owner
			this->remove_character(character);
			home_province_owner->get_game_data()->add_character(character);
		}
	}

	for (const province *province : this->get_provinces()) {
		for (const character *character : province->get_characters()) {
			if (character->get_game_data()->get_employer() != nullptr) {
				continue;
			}

			if (date >= character->get_start_date() && date < character->get_end_date()) {
				this->add_character(character);
			}
		}
	}

	//check if the offices of characters are still valid, and if not remove the character from that office
	const office_map<const character *> office_characters = this->office_characters;
	const read_only_context ctx = read_only_context::from_scope(this->country);
	for (const auto &[office, character] : office_characters) {
		const condition<metternich::country> *country_conditions = office->get_country_conditions();
		if (country_conditions != nullptr && !country_conditions->check(this->country, ctx)) {
			this->set_office_character(office, nullptr);
		}
	}

	this->fill_empty_offices();
}

void country_game_data::add_character(const character *character)
{
	this->characters.push_back(character);

	character->get_game_data()->set_employer(this->country);

	this->sort_characters();

	emit characters_changed();
}

void country_game_data::remove_character(const character *character)
{
	std::erase(this->characters, character);

	if (character == this->get_ruler()) {
		this->set_ruler(nullptr);
	}

	if (character->get_game_data()->get_office() != nullptr) {
		this->set_office_character(character->get_game_data()->get_office(), nullptr);
	}

	character->get_game_data()->set_employer(nullptr);

	this->sort_characters();

	emit characters_changed();
}

void country_game_data::clear_characters()
{
	const std::vector<const character *> characters = this->get_characters();
	for (const character *character : characters) {
		this->remove_character(character);
	}

	assert_throw(this->get_characters().empty());

	emit characters_changed();
}

void country_game_data::sort_characters()
{
	std::sort(this->characters.begin(), this->characters.end(), [](const character *lhs, const character *rhs) {
		const character_game_data *lhs_game_data = lhs->get_game_data();
		const character_game_data *rhs_game_data = rhs->get_game_data();

		if (lhs_game_data->is_ruler() != rhs_game_data->is_ruler()) {
			return lhs_game_data->is_ruler();
		}

		if (lhs_game_data->get_office() != rhs_game_data->get_office()) {
			if (lhs_game_data->get_office() == nullptr || rhs_game_data->get_office() == nullptr) {
				return lhs_game_data->get_office() != nullptr;
			}

			if (lhs_game_data->get_office()->get_type() != rhs_game_data->get_office()->get_type()) {
				return lhs_game_data->get_office()->get_type() < rhs_game_data->get_office()->get_type();
			}

			return lhs_game_data->get_office()->get_identifier() < rhs_game_data->get_office()->get_identifier();
		}

		if (lhs_game_data->get_primary_attribute_value() != rhs_game_data->get_primary_attribute_value()) {
			return lhs_game_data->get_primary_attribute_value() > rhs_game_data->get_primary_attribute_value();
		}

		if (lhs->get_birth_date() != rhs->get_birth_date()) {
			return lhs->get_birth_date() < rhs->get_birth_date();
		}

		return lhs->get_identifier() < rhs->get_identifier();
	});
}

void country_game_data::set_ruler(const character *ruler)
{
	if (ruler == this->get_ruler()) {
		return;
	}

	if (this->get_ruler() != nullptr) {
		this->apply_ruler_effects(-1);
	}

	this->ruler = ruler;

	assert_throw(this->country->get_title() != nullptr);
	this->country->get_title()->get_game_data()->set_holder(this->ruler);

	if (this->get_ruler() != nullptr) {
		this->apply_ruler_effects(1);
	}

	this->sort_characters();

	if (game::get()->is_running()) {
		emit ruler_changed();
	}
}

void country_game_data::apply_ruler_effects(const int multiplier)
{
	assert_throw(this->get_ruler() != nullptr);

	character_game_data *ruler_game_data = this->get_ruler()->get_game_data();

	ruler_game_data->apply_country_modifier(this->country, multiplier);
	this->change_quarterly_prestige(ruler_game_data->get_quarterly_prestige() * multiplier);
	this->change_quarterly_piety(ruler_game_data->get_quarterly_piety() * multiplier);
}

QObject *country_game_data::get_office_character(metternich::office *office_param) const
{
	const office *office = office_param;
	return const_cast<metternich::character *>(this->get_office_character(office));
}

void country_game_data::set_office_character(const office *office, const character *character)
{
	const metternich::character *old_character = this->get_office_character(office);

	if (character == old_character) {
		return;
	}

	if (old_character != nullptr) {
		old_character->get_game_data()->apply_country_modifier(this->country, -1);
		old_character->get_game_data()->set_office(nullptr);
	}

	if (character != nullptr) {
		this->office_characters[office] = character;
	} else {
		this->office_characters.erase(office);
	}

	if (character != nullptr) {
		character->get_game_data()->apply_country_modifier(this->country, 1);
		character->get_game_data()->set_office(office);
	}

	this->sort_characters();

	if (game::get()->is_running()) {
		emit office_characters_changed();
	}
}

std::vector<const office *> country_game_data::get_offices() const
{
	std::vector<const office *> offices;

	const read_only_context ctx = read_only_context::from_scope(this->country);

	for (const office *office : office::get_all()) {
		if (office->get_type() != office_type::country) {
			continue;
		}

		if (office == defines::get()->get_head_of_government_office()) {
			continue;
		}

		const condition<metternich::country> *country_conditions = office->get_country_conditions();
		if (country_conditions != nullptr && !country_conditions->check(this->country, ctx)) {
			continue;
		}

		offices.push_back(office);
	}

	std::sort(offices.begin(), offices.end(), [](const office *lhs, const office *rhs) {
		return lhs->get_identifier() < rhs->get_identifier();
	});

	return offices;
}

QVariantList country_game_data::get_offices_qvariant_list() const
{
	return container::to_qvariant_list(this->get_offices());
}

void country_game_data::fill_empty_offices()
{
	const std::vector<const office *> offices = this->get_offices();

	for (const office *office : offices) {
		if (this->get_office_character(office) != nullptr) {
			continue;
		}

		int best_skill = 0;
		std::vector<const character *> potential_characters;

		for (const character *character : this->get_characters()) {
			const int skill = character->get_game_data()->get_primary_attribute_value();

			if (skill < best_skill) {
				continue;
			}

			if (office->get_character_conditions() != nullptr && !office->get_character_conditions()->check(character, read_only_context::from_scope(character))) {
				continue;
			}

			if (skill > best_skill) {
				best_skill = skill;
				potential_characters.clear();
			}

			potential_characters.push_back(character);
		}

		if (!potential_characters.empty()) {
			this->set_office_character(office, vector::get_random(potential_characters));
		}
	}
}

void country_game_data::add_civilian_unit(qunique_ptr<metternich::civilian_unit> &&civilian_unit)
{
	this->civilian_units.push_back(std::move(civilian_unit));
}

void country_game_data::remove_civilian_unit(metternich::civilian_unit *civilian_unit)
{
	for (size_t i = 0; i < this->civilian_units.size(); ++i) {
		if (this->civilian_units[i].get() == civilian_unit) {
			this->civilian_units.erase(this->civilian_units.begin() + i);
			return;
		}
	}
}

}
