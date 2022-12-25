#include "metternich.h"

#include "country/country_game_data.h"

#include "country/country.h"
#include "country/country_type.h"
#include "country/culture.h"
#include "country/diplomacy_state.h"
#include "database/defines.h"
#include "database/preferences.h"
#include "economy/commodity.h"
#include "economy/resource.h"
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
	this->do_migration();

	for (const province *province : this->get_provinces()) {
		province->get_game_data()->do_turn();
	}

	this->do_population_growth();

	for (const qunique_ptr<civilian_unit> &civilian_unit : this->civilian_units) {
		civilian_unit->do_turn();
	}
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

	for (const auto &[population_type, count] : province_game_data->get_population_type_counts()) {
		this->change_population_type_count(population_type, count);
	}
	for (const auto &[culture, count] : province_game_data->get_population_culture_counts()) {
		this->change_population_culture_count(culture, count);
	}
	for (const auto &[phenotype, count] : province_game_data->get_population_phenotype_counts()) {
		this->change_population_phenotype_count(phenotype, count);
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

	for (const auto &[population_type, count] : province_game_data->get_population_type_counts()) {
		this->change_population_type_count(population_type, -count);
	}
	for (const auto &[culture, count] : province_game_data->get_population_culture_counts()) {
		this->change_population_culture_count(culture, -count);
	}
	for (const auto &[phenotype, count] : province_game_data->get_population_phenotype_counts()) {
		this->change_population_phenotype_count(phenotype, -count);
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

	if (value == 0) {
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
