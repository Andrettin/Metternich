#include "metternich.h"

#include "domain/domain_diplomacy.h"

#include "culture/culture.h"
#include "database/defines.h"
#include "domain/consulate.h"
#include "domain/diplomacy_state.h"
#include "domain/domain.h"
#include "domain/domain_economy.h"
#include "domain/domain_game_data.h"
#include "domain/domain_history.h"
#include "domain/domain_technology.h"
#include "domain/domain_turn_data.h"
#include "domain/subject_type.h"
#include "game/game.h"
#include "map/diplomatic_map_mode.h"
#include "map/map.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/province_map_data.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "religion/religion.h"
#include "script/opinion_modifier.h"
#include "util/assert_util.h"
#include "util/image_util.h"
#include "util/map_util.h"

#include "xbrz.h"

#include <magic_enum/magic_enum.hpp>

namespace metternich {

domain_diplomacy::domain_diplomacy(const metternich::domain *domain, const domain_game_data *game_data)
	: domain(domain)
{
	connect(this, &domain_diplomacy::overlord_changed, game_data, &domain_game_data::realm_changed);
}

domain_diplomacy::~domain_diplomacy()
{
}

void domain_diplomacy::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "overlord") {
		this->overlord = domain::get(value);
	} else if (key == "subject_type") {
		this->subject_type = subject_type::get(value);
	} else {
		throw std::runtime_error(std::format("Invalid domain diplomacy property: \"{}\".", key));
	}
}

void domain_diplomacy::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "base_opinions") {
		scope.for_each_property([this](const gsml_property &property) {
			this->base_opinions[domain::get(property.get_key())] = std::stoi(property.get_value());
		});
	} else {
		throw std::runtime_error(std::format("Invalid domain diplomacy scope: \"{}\".", tag));
	}
}

gsml_data domain_diplomacy::to_gsml_data() const
{
	gsml_data data("diplomacy");

	if (this->get_overlord() != nullptr) {
		data.add_property("overlord", this->get_overlord()->get_identifier());
	}

	if (this->get_subject_type() != nullptr) {
		data.add_property("subject_type", this->get_subject_type()->get_identifier());
	}

	if (!this->base_opinions.empty()) {
		gsml_data base_opinions_data("base_opinions");
		for (const auto &[domain, base_opinion] : this->base_opinions) {
			base_opinions_data.add_property(domain->get_identifier(), std::to_string(base_opinion));
		}
		data.add_child(std::move(base_opinions_data));
	}

	return data;
}

domain_game_data *domain_diplomacy::get_game_data() const
{
	return this->domain->get_game_data();
}

QCoro::Task<void> domain_diplomacy::apply_diplomatic_history()
{
	const domain_history *domain_history = this->domain->get_history();

	for (const auto &[other_domain, diplomacy_state] : domain_history->get_diplomacy_states()) {
		if (!other_domain->get_game_data()->is_alive()) {
			continue;
		}

		co_await this->set_diplomacy_state(other_domain, diplomacy_state);
		co_await other_domain->get_diplomacy()->set_diplomacy_state(this->domain, get_diplomacy_state_counterpart(diplomacy_state));
	}

	for (const auto &[other_country, consulate] : domain_history->get_consulates()) {
		if (!other_country->get_game_data()->is_alive()) {
			continue;
		}

		this->set_consulate(other_country, consulate);
		other_country->get_diplomacy()->set_consulate(this->domain, consulate);
	}
}

QCoro::Task<void> domain_diplomacy::set_overlord(const metternich::domain *overlord)
{
	if (overlord == this->get_overlord()) {
		co_return;
	}

	if (overlord != nullptr && overlord->get_game_data()->get_tier() <= this->get_game_data()->get_tier()) {
		throw std::runtime_error(std::format("Tried to set \"{}\" as the overlord of \"{}\", but the former does not have a higher tier than the latter.", overlord->get_identifier(), this->domain->get_identifier()));
	}

	if (this->get_overlord() != nullptr) {
		this->get_overlord()->get_game_data()->change_economic_score(-this->get_game_data()->get_economic_score() * this->get_subject_type()->get_wealth_tribute_rate() / 100);

		for (const auto &[resource, count] : this->domain->get_economy()->get_resource_counts()) {
			this->get_overlord()->get_economy()->change_vassal_resource_count(resource, -count);
		}
	}

	this->overlord = overlord;

	if (this->get_overlord() != nullptr) {
		this->get_overlord()->get_game_data()->change_economic_score(this->get_game_data()->get_economic_score() * this->get_subject_type()->get_wealth_tribute_rate() / 100);

		for (const auto &[resource, count] : this->domain->get_economy()->get_resource_counts()) {
			this->get_overlord()->get_economy()->change_vassal_resource_count(resource, count);
		}
	} else {
		co_await this->set_subject_type(nullptr);
	}

	if (game::get()->is_running()) {
		emit overlord_changed();
	}
}

bool domain_diplomacy::is_vassal_of(const metternich::domain *domain) const
{
	return this->get_overlord() == domain;
}

bool domain_diplomacy::is_any_vassal_of(const metternich::domain *domain) const
{
	if (this->is_vassal_of(domain)) {
		return true;
	}

	if (this->get_overlord() != nullptr) {
		return this->get_overlord()->get_diplomacy()->is_any_vassal_of(domain);
	}

	return false;
}

bool domain_diplomacy::is_overlord_of(const metternich::domain *domain) const
{
	return domain->get_diplomacy()->is_vassal_of(this->domain);
}

bool domain_diplomacy::is_any_overlord_of(const metternich::domain *domain) const
{
	if (this->is_overlord_of(domain)) {
		return true;
	}

	const std::vector<const metternich::domain *> vassals = this->get_vassals();
	for (const metternich::domain *vassal : this->get_vassals()) {
		if (vassal->get_diplomacy()->is_any_overlord_of(domain)) {
			return true;
		}
	}

	return false;
}

QCoro::Task<void> domain_diplomacy::set_subject_type(const metternich::subject_type *subject_type)
{
	if (subject_type == this->get_subject_type()) {
		co_return;
	}

	this->subject_type = subject_type;

	if (game::get()->is_running()) {
		emit subject_type_changed();
	}

	co_await this->get_game_data()->check_government_type();
}

QCoro::Task<void> domain_diplomacy::add_known_country(const metternich::domain *other_domain)
{
	this->known_countries.insert(other_domain);

	const consulate *current_consulate = this->get_consulate(other_domain);

	const consulate *best_free_consulate = nullptr;
	for (const auto &[consulate, count] : this->free_consulate_counts) {
		if (best_free_consulate == nullptr || consulate->get_level() > best_free_consulate->get_level()) {
			best_free_consulate = consulate;
		}
	}

	if (best_free_consulate != nullptr && (current_consulate == nullptr || current_consulate->get_level() < best_free_consulate->get_level())) {
		this->set_consulate(other_domain, best_free_consulate);
	}

	if (this->get_game_data()->get_technology()->get_gain_technologies_known_by_others_count() > 0) {
		co_await this->get_game_data()->get_technology()->gain_technologies_known_by_others();
	}
}

diplomacy_state domain_diplomacy::get_diplomacy_state(const metternich::domain *other_domain) const
{
	const auto find_iterator = this->diplomacy_states.find(other_domain);

	if (find_iterator != this->diplomacy_states.end()) {
		return find_iterator->second;
	}

	return diplomacy_state::peace;
}

QCoro::Task<void> domain_diplomacy::set_diplomacy_state(const metternich::domain *other_domain, const diplomacy_state state)
{
	const diplomacy_state old_state = this->get_diplomacy_state(other_domain);

	if (state == old_state) {
		co_return;
	}

	if (is_vassalage_diplomacy_state(state)) {
		co_await this->set_overlord(other_domain);
	} else {
		if (this->get_overlord() == other_domain) {
			co_await this->set_overlord(nullptr);
		}
	}

	if (old_state != diplomacy_state::peace) {
		this->change_diplomacy_state_count(old_state, -1);
	}

	if (state == diplomacy_state::peace) {
		this->diplomacy_states.erase(other_domain);
	} else {
		this->diplomacy_states[other_domain] = state;
		this->change_diplomacy_state_count(state, 1);
	}

	if (is_overlordship_diplomacy_state(old_state) || is_overlordship_diplomacy_state(state)) {
		if (game::get()->is_loaded()) {
			this->get_game_data()->calculate_realm_territory_rect();
		}
	}

	if (game::get()->is_running()) {
		emit diplomacy_states_changed();

		if (is_vassalage_diplomacy_state(state) || is_vassalage_diplomacy_state(old_state)) {
			emit this->get_game_data()->type_name_changed();
		}
	}
}

void domain_diplomacy::change_diplomacy_state_count(const diplomacy_state state, const int change)
{
	const int final_count = (this->diplomacy_state_counts[state] += change);

	if (final_count == 0) {
		this->diplomacy_state_counts.erase(state);
		this->diplomacy_state_diplomatic_map_image_promises.erase(state);
	}

	//if the change added the diplomacy state to the map, then we need to create the diplomatic map image for it
	if (game::get()->is_running() && final_count == change && !is_vassalage_diplomacy_state(state) && !is_overlordship_diplomacy_state(state)) {
		this->domain->get_turn_data()->set_diplomatic_map_diplomacy_state_dirty(state);
	}
}

QString domain_diplomacy::get_diplomacy_state_diplomatic_map_suffix(metternich::domain *other_domain) const
{
	if (other_domain == this->domain || this->is_any_overlord_of(other_domain) || this->is_any_vassal_of(other_domain)) {
		return "empire";
	}

	return QString::fromStdString(std::string(magic_enum::enum_name(this->get_diplomacy_state(other_domain))));
}

bool domain_diplomacy::at_war() const
{
	return this->diplomacy_state_counts.contains(diplomacy_state::war);
}

bool domain_diplomacy::can_attack(const metternich::domain *other_domain) const
{
	if (other_domain == nullptr) {
		return false;
	}

	if (other_domain == this->domain) {
		return false;
	}

	if (this->is_any_overlord_of(other_domain)) {
		return false;
	}

	if (other_domain->is_clade()) {
		return true;
	} else if (this->get_game_data()->is_clade()) {
		return false;
	}

	switch (this->get_diplomacy_state(other_domain)) {
		case diplomacy_state::non_aggression_pact:
		case diplomacy_state::alliance:
			return false;
		case diplomacy_state::war:
			return true;
		default:
			break;
	}

	if (other_domain->get_game_data()->is_tribal() || this->get_game_data()->is_tribal()) {
		return true;
	}

	if (other_domain->get_game_data()->is_under_anarchy() || this->get_game_data()->is_under_anarchy()) {
		return true;
	}

	return false;
}

std::optional<diplomacy_state> domain_diplomacy::get_offered_diplomacy_state(const metternich::domain *other_domain) const
{
	const auto find_iterator = this->offered_diplomacy_states.find(other_domain);

	if (find_iterator != this->offered_diplomacy_states.end()) {
		return find_iterator->second;
	}

	return std::nullopt;
}

void domain_diplomacy::set_offered_diplomacy_state(const metternich::domain *other_domain, const std::optional<diplomacy_state> &state)
{
	const diplomacy_state old_state = this->get_diplomacy_state(other_domain);

	if (state == old_state) {
		return;
	}

	if (state.has_value()) {
		this->offered_diplomacy_states[other_domain] = state.value();
	} else {
		this->offered_diplomacy_states.erase(other_domain);
	}

	if (game::get()->is_running()) {
		emit offered_diplomacy_states_changed();
	}
}

QVariantList domain_diplomacy::get_consulates_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->consulates);
}

void domain_diplomacy::set_consulate(const metternich::domain *other_domain, const consulate *consulate)
{
	if (consulate == nullptr) {
		this->consulates.erase(other_domain);
	} else {
		this->consulates[other_domain] = consulate;

		if (other_domain->get_diplomacy()->get_consulate(this->domain) != consulate) {
			other_domain->get_diplomacy()->set_consulate(this->domain, consulate);
		}
	}

	if (game::get()->is_running()) {
		emit consulates_changed();
	}
}

int domain_diplomacy::get_opinion_of(const metternich::domain *other) const
{
	int opinion = this->get_base_opinion(other);

	for (const auto &[modifier, duration] : this->get_opinion_modifiers_for(other)) {
		opinion += modifier->get_value();
	}

	opinion = std::clamp(opinion, domain::min_opinion, domain::max_opinion);

	return opinion;
}

void domain_diplomacy::set_base_opinion(const metternich::domain *other, const int opinion)
{
	assert_throw(other != this->domain);

	if (opinion == this->get_base_opinion(other)) {
		return;
	}

	if (opinion < domain::min_opinion) {
		this->set_base_opinion(other, domain::min_opinion);
		return;
	} else if (opinion > domain::max_opinion) {
		this->set_base_opinion(other, domain::max_opinion);
		return;
	}

	if (opinion == 0) {
		this->base_opinions.erase(other);
	} else {
		this->base_opinions[other] = opinion;
	}
}

void domain_diplomacy::add_opinion_modifier(const metternich::domain *other, const opinion_modifier *modifier, const int duration)
{
	this->opinion_modifiers[other][modifier] = std::max(this->opinion_modifiers[other][modifier], duration);
}

void domain_diplomacy::remove_opinion_modifier(const metternich::domain *other, const opinion_modifier *modifier)
{
	opinion_modifier_map<int> &opinion_modifiers = this->opinion_modifiers[other];
	opinion_modifiers.erase(modifier);

	if (opinion_modifiers.empty()) {
		this->opinion_modifiers.erase(other);
	}
}

void domain_diplomacy::decrement_opinion_modifiers()
{
	domain_map<std::vector<const opinion_modifier *>> opinion_modifiers_to_remove;

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

std::vector<const metternich::domain *> domain_diplomacy::get_vassals() const
{
	std::vector<const metternich::domain *> vassals;

	for (const auto &[domain, diplomacy_state] : this->diplomacy_states) {
		if (is_overlordship_diplomacy_state(diplomacy_state)) {
			vassals.push_back(domain);
		}
	}

	return vassals;
}

QVariantList domain_diplomacy::get_vassals_qvariant_list() const
{
	return container::to_qvariant_list(this->get_vassals());
}

QVariantList domain_diplomacy::get_subject_type_counts_qvariant_list() const
{
	std::map<const metternich::subject_type *, int> subject_type_counts;

	for (const auto &[country, diplomacy_state] : this->diplomacy_states) {
		if (is_overlordship_diplomacy_state(diplomacy_state)) {
			assert_throw(country->get_diplomacy()->get_subject_type() != nullptr);
			++subject_type_counts[country->get_diplomacy()->get_subject_type()];
		}
	}

	QVariantList counts = archimedes::map::to_qvariant_list(subject_type_counts);
	std::sort(counts.begin(), counts.end(), [](const QVariant &lhs, const QVariant &rhs) {
		return lhs.toMap().value("value").toInt() > rhs.toMap().value("value").toInt();
	});

	return counts;
}

const QColor &domain_diplomacy::get_diplomatic_map_color() const
{
	if (this->get_overlord() != nullptr) {
		return this->get_overlord()->get_diplomacy()->get_diplomatic_map_color();
	}

	return this->domain->get_color();
}

QImage domain_diplomacy::prepare_diplomatic_map_image() const
{
	assert_throw(this->get_game_data()->get_territory_rect().width() > 0);
	assert_throw(this->get_game_data()->get_territory_rect().height() > 0);

	const decimillesimal_int &tile_scale = map::get()->get_diplomatic_map_tile_scale();
	QSize image_size;
	if (tile_scale < 1) {
		image_size = QSize((this->get_game_data()->get_territory_rect().width() * tile_scale).to_ceil_int(), (this->get_game_data()->get_territory_rect().height() * tile_scale).to_ceil_int());
	} else {
		image_size = this->get_game_data()->get_territory_rect().size();
	}

	QImage image(image_size, QImage::Format_RGBA8888);
	image.fill(Qt::transparent);

	return image;
}

QImage domain_diplomacy::finalize_diplomatic_map_image(QImage &&image)
{
	assert_throw(!image.isNull());

	const decimillesimal_int &tile_scale = map::get()->get_diplomatic_map_tile_scale();

	if (tile_scale > 1) {
		QImage scaled_image;

		scaled_image = image::scale<QImage::Format_ARGB32>(image, centesimal_int(tile_scale), [](const size_t factor, const uint32_t *src, uint32_t *tgt, const int src_width, const int src_height) {
			xbrz::scale(factor, src, tgt, src_width, src_height, xbrz::ColorFormat::ARGB);
		});

		image = std::move(scaled_image);
	}

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

	return image;
}

void domain_diplomacy::create_diplomatic_map_image()
{
	if (this->get_game_data()->get_provinces().empty()) {
		return;
	}

	const map *map = map::get();

	QImage diplomatic_map_image = this->prepare_diplomatic_map_image();
	QImage selected_diplomatic_map_image = diplomatic_map_image;

	const QColor &color = this->get_diplomatic_map_color();
	const QColor &selected_color = defines::get()->get_selected_country_color();

	const decimillesimal_int &tile_scale = map::get()->get_diplomatic_map_tile_scale();
	const QPoint top_left = this->get_game_data()->get_territory_rect().topLeft() * tile_scale;

	const QSize image_size = diplomatic_map_image.size();

	//normalize the tile top left
	const QPoint tile_top_left = this->get_game_data()->get_territory_rect().topLeft() * tile_scale / tile_scale;

	for (int x = 0; x < image_size.width(); ++x) {
		for (int y = 0; y < image_size.height(); ++y) {
			const QPoint pixel_pos = QPoint(x, y);
			const QPoint relative_tile_pos = tile_scale < 1 ? pixel_pos / tile_scale : pixel_pos;
			const tile *tile = map->get_tile(tile_top_left + relative_tile_pos);

			if (tile->get_owner() != this->domain) {
				continue;
			}

			diplomatic_map_image.setPixelColor(pixel_pos, color);
			selected_diplomatic_map_image.setPixelColor(pixel_pos, selected_color);
		}
	}

	std::shared_ptr<QPromise<QImage>> promise = std::make_shared<QPromise<QImage>>();
	this->diplomatic_map_image_promise = promise;
	this->diplomatic_map_image_promise->start();
	assert_throw(!diplomatic_map_image.isNull());
	QThreadPool::globalInstance()->start([promise, image = std::move(diplomatic_map_image)]() mutable {
		promise->addResult(domain_diplomacy::finalize_diplomatic_map_image(std::move(image)));
		promise->finish();
	});

	std::shared_ptr<QPromise<QImage>> selected_promise = std::make_shared<QPromise<QImage>>();
	this->selected_diplomatic_map_image_promise = selected_promise;
	this->selected_diplomatic_map_image_promise->start();
	assert_throw(!selected_diplomatic_map_image.isNull());
	QThreadPool::globalInstance()->start([selected_promise, image = std::move(selected_diplomatic_map_image)]() mutable {
		selected_promise->addResult(domain_diplomacy::finalize_diplomatic_map_image(std::move(image)));
		selected_promise->finish();
	});

	this->diplomatic_map_image_rect = QRect(top_left, image_size);

	this->create_diplomatic_map_mode_image(diplomatic_map_mode::diplomatic);
	this->create_diplomacy_state_diplomatic_map_image(diplomacy_state::peace);

	for (const auto &[diplomacy_state, count] : this->get_diplomacy_state_counts()) {
		if (!is_vassalage_diplomacy_state(diplomacy_state) && !is_overlordship_diplomacy_state(diplomacy_state)) {
			this->create_diplomacy_state_diplomatic_map_image(diplomacy_state);
		}
	}

	this->create_diplomatic_map_mode_image(diplomatic_map_mode::terrain);
	this->create_diplomatic_map_mode_image(diplomatic_map_mode::cultural);
	this->create_diplomatic_map_mode_image(diplomatic_map_mode::religious);
	this->create_diplomatic_map_mode_image(diplomatic_map_mode::trade_zone);
	this->create_diplomatic_map_mode_image(diplomatic_map_mode::temple);

	if (game::get()->is_running()) {
		emit diplomatic_map_image_changed();
	}
}

QImage domain_diplomacy::prepare_realm_diplomatic_map_image() const
{
	assert_throw(this->get_game_data()->get_realm_territory_rect().width() > 0);
	assert_throw(this->get_game_data()->get_realm_territory_rect().height() > 0);

	const decimillesimal_int &tile_scale = map::get()->get_diplomatic_map_tile_scale();
	QSize image_size;
	if (tile_scale < 1) {
		image_size = QSize((this->get_game_data()->get_realm_territory_rect().width() * tile_scale).to_ceil_int(), (this->get_game_data()->get_realm_territory_rect().height() * tile_scale).to_ceil_int());
	} else {
		image_size = this->get_game_data()->get_realm_territory_rect().size();
	}

	QImage image(image_size, QImage::Format_RGBA8888);
	image.fill(Qt::transparent);

	return image;
}

void domain_diplomacy::create_realm_diplomatic_map_image()
{
	if (!this->is_independent() || this->get_game_data()->get_provinces().empty()) {
		return;
	}

	if (this->get_vassals().empty() && this->get_diplomatic_map_image_promise() != nullptr && this->get_selected_diplomatic_map_image_promise() != nullptr) {
		this->realm_diplomatic_map_image_promise = this->diplomatic_map_image_promise;
		this->selected_realm_diplomatic_map_image_promise = this->selected_diplomatic_map_image_promise;
		this->realm_diplomatic_map_image_rect = this->get_diplomatic_map_image_rect();
		return;
	}

	const map *map = map::get();

	QImage diplomatic_map_image = this->prepare_realm_diplomatic_map_image();
	QImage selected_diplomatic_map_image = diplomatic_map_image;

	const QColor &color = this->get_diplomatic_map_color();
	const QColor &selected_color = defines::get()->get_selected_country_color();

	const decimillesimal_int &tile_scale = map::get()->get_diplomatic_map_tile_scale();
	const QPoint top_left = this->get_game_data()->get_realm_territory_rect().topLeft() * tile_scale;

	const QSize image_size = diplomatic_map_image.size();

	//normalize the tile top left
	const QPoint tile_top_left = this->get_game_data()->get_realm_territory_rect().topLeft() * tile_scale / tile_scale;

	for (int x = 0; x < image_size.width(); ++x) {
		for (int y = 0; y < image_size.height(); ++y) {
			const QPoint pixel_pos = QPoint(x, y);
			const QPoint relative_tile_pos = tile_scale < 1 ? pixel_pos / tile_scale : pixel_pos;
			const tile *tile = map->get_tile(tile_top_left + relative_tile_pos);

			if (tile->get_owner() == nullptr || tile->get_owner()->get_game_data()->get_realm() != this->domain) {
				continue;
			}

			diplomatic_map_image.setPixelColor(pixel_pos, color);
			selected_diplomatic_map_image.setPixelColor(pixel_pos, selected_color);
		}
	}

	std::shared_ptr<QPromise<QImage>> promise = std::make_shared<QPromise<QImage>>();
	this->realm_diplomatic_map_image_promise = promise;
	this->realm_diplomatic_map_image_promise->start();
	QThreadPool::globalInstance()->start([promise, image = std::move(diplomatic_map_image)]() mutable {
		promise->addResult(domain_diplomacy::finalize_diplomatic_map_image(std::move(image)));
		promise->finish();
	});

	std::shared_ptr<QPromise<QImage>> selected_promise = std::make_shared<QPromise<QImage>>();
	this->selected_realm_diplomatic_map_image_promise = selected_promise;
	this->selected_realm_diplomatic_map_image_promise->start();
	QThreadPool::globalInstance()->start([selected_promise, image = std::move(selected_diplomatic_map_image)]() mutable {
		selected_promise->addResult(domain_diplomacy::finalize_diplomatic_map_image(std::move(image)));
		selected_promise->finish();
	});

	this->realm_diplomatic_map_image_rect = QRect(top_left, image_size);

	if (game::get()->is_running()) {
		emit realm_diplomatic_map_image_changed();
	}
}

void domain_diplomacy::create_diplomatic_map_mode_image(const diplomatic_map_mode mode)
{
	static const QColor empty_color(Qt::black);
	static constexpr QColor diplomatic_self_color(170, 148, 214);

	const map *map = map::get();

	QImage image = this->prepare_diplomatic_map_image();

	const decimillesimal_int &tile_scale = map::get()->get_diplomatic_map_tile_scale();
	const QPoint top_left = this->get_game_data()->get_territory_rect().topLeft() * tile_scale;

	const QSize image_size = image.size();

	//normalize the tile top left
	const QPoint tile_top_left = this->get_game_data()->get_territory_rect().topLeft() * tile_scale / tile_scale;

	for (int x = 0; x < image_size.width(); ++x) {
		for (int y = 0; y < image_size.height(); ++y) {
			const QPoint pixel_pos = QPoint(x, y);
			const QPoint relative_tile_pos = tile_scale < 1 ? pixel_pos / tile_scale : pixel_pos;
			const tile *tile = map->get_tile(tile_top_left + relative_tile_pos);

			if (tile->get_owner() != this->domain) {
				continue;
			}

			const QColor *color = nullptr;

			switch (mode) {
				case diplomatic_map_mode::diplomatic:
					color = &diplomatic_self_color;
					break;
				case diplomatic_map_mode::terrain:
					color = &tile->get_province()->get_map_data()->get_terrain()->get_color();
					break;
				case diplomatic_map_mode::cultural: {
					const metternich::culture *culture = tile->get_province()->get_game_data()->get_culture();

					if (culture != nullptr) {
						color = &culture->get_color();
					} else {
						color = &defines::get()->get_map_blank_color();
					}
					break;
				}
				case diplomatic_map_mode::religious: {
					const metternich::religion *religion = tile->get_province()->get_game_data()->get_religion();

					if (religion != nullptr) {
						color = &religion->get_color();
					} else {
						color = &defines::get()->get_map_blank_color();
					}
					break;
				}
				case diplomatic_map_mode::trade_zone: {
					const metternich::domain *trade_zone_domain = tile->get_province()->get_game_data()->get_trade_zone_domain();

					if (trade_zone_domain != nullptr) {
						color = &trade_zone_domain->get_diplomacy()->get_diplomatic_map_color();
					} else {
						color = &defines::get()->get_map_blank_color();
					}
					break;
				}
				case diplomatic_map_mode::temple: {
					const metternich::domain *temple_domain = tile->get_province()->get_game_data()->get_temple_domain();

					if (temple_domain != nullptr) {
						color = &temple_domain->get_diplomacy()->get_diplomatic_map_color();
					} else {
						color = &defines::get()->get_map_blank_color();
					}
					break;
				}
			}

			image.setPixelColor(pixel_pos, *color);
		}
	}

	std::shared_ptr<QPromise<QImage>> promise = std::make_shared<QPromise<QImage>>();
	this->diplomatic_map_mode_image_promises[mode] = promise;
	promise->start();

	QThreadPool::globalInstance()->start([promise, image = std::move(image)]() mutable {
		promise->addResult(domain_diplomacy::finalize_diplomatic_map_image(std::move(image)));
		promise->finish();
	});
}

void domain_diplomacy::create_diplomacy_state_diplomatic_map_image(const diplomacy_state state)
{
	static const QColor empty_color(Qt::black);

	const map *map = map::get();

	QImage image = this->prepare_diplomatic_map_image();

	const decimillesimal_int &tile_scale = map::get()->get_diplomatic_map_tile_scale();
	const QPoint top_left = this->get_game_data()->get_territory_rect().topLeft() * tile_scale;

	const QSize image_size = image.size();

	//normalize the tile top left
	const QPoint tile_top_left = this->get_game_data()->get_territory_rect().topLeft() * tile_scale / tile_scale;

	for (int x = 0; x < image_size.width(); ++x) {
		for (int y = 0; y < image_size.height(); ++y) {
			const QPoint pixel_pos = QPoint(x, y);
			const QPoint relative_tile_pos = tile_scale < 1 ? pixel_pos / tile_scale : pixel_pos;
			const tile *tile = map->get_tile(tile_top_left + relative_tile_pos);

			if (tile->get_owner() != this->domain) {
				continue;
			}

			const QColor &color = defines::get()->get_diplomacy_state_color(state);

			image.setPixelColor(pixel_pos, color);
		}
	}

	std::shared_ptr<QPromise<QImage>> promise = std::make_shared<QPromise<QImage>>();
	this->diplomacy_state_diplomatic_map_image_promises[state] = promise;
	promise->start();

	QThreadPool::globalInstance()->start([promise, image = std::move(image)]() mutable {
		promise->addResult(domain_diplomacy::finalize_diplomatic_map_image(std::move(image)));
		promise->finish();
	});
}

bool domain_diplomacy::can_declare_war_on(const metternich::domain *other_domain) const
{
	if (!this->domain->can_declare_war()) {
		return false;
	}

	if (this->get_overlord() != nullptr) {
		return other_domain == this->get_overlord();
	}

	return true;
}

void domain_diplomacy::set_free_consulate_count(const consulate *consulate, const int value)
{
	const int old_value = this->get_free_consulate_count(consulate);
	if (value == old_value) {
		return;
	}

	assert_throw(value >= 0);

	if (value == 0) {
		this->free_consulate_counts.erase(consulate);
	} else if (old_value == 0) {
		this->free_consulate_counts[consulate] = value;

		for (const metternich::domain *known_country : this->get_known_countries()) {
			const metternich::consulate *current_consulate = this->get_consulate(known_country);
			if (current_consulate == nullptr || current_consulate->get_level() < consulate->get_level()) {
				this->set_consulate(known_country, consulate);
			}
		}
	}
}

}
