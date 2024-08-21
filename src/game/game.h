#pragma once

#include "economy/commodity_container.h"
#include "util/qunique_ptr.h"
#include "util/singleton.h"

Q_MOC_INCLUDE("country/country.h")

namespace archimedes {
	class gsml_data;
	class gsml_property;
}

namespace metternich {

class army;
class character;
class country;
class era;
class game_rules;
class province;
class scenario;
class site;
class wonder;
struct population_group_key;

template <typename scope_type>
class delayed_effect_instance;

class game final : public QObject, public singleton<game>
{
	Q_OBJECT

	Q_PROPERTY(bool running READ is_running NOTIFY running_changed)
	Q_PROPERTY(QDate date READ get_date NOTIFY turn_changed)
	Q_PROPERTY(int year READ get_year NOTIFY turn_changed)
	Q_PROPERTY(QString date_string READ get_date_qstring NOTIFY turn_changed)
	Q_PROPERTY(int turn READ get_turn NOTIFY turn_changed)
	Q_PROPERTY(QVariantList countries READ get_countries_qvariant_list NOTIFY countries_changed)
	Q_PROPERTY(QVariantList great_powers READ get_great_powers_qvariant_list NOTIFY countries_changed)
	Q_PROPERTY(const metternich::country* player_country READ get_player_country WRITE set_player_country NOTIFY player_country_changed)
	Q_PROPERTY(const metternich::game_rules* rules READ get_rules CONSTANT)

public:
	static QDate normalize_date(const QDate &date);

	game();
	~game();

	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);

	gsml_data to_gsml_data() const;

	const game_rules *get_rules() const
	{
		return this->rules.get();
	}

	bool is_running() const
	{
		return this->running;
	}

	void set_running(const bool running)
	{
		if (running == this->is_running()) {
			return;
		}

		this->running = running;

		emit running_changed();
	}

	const metternich::scenario *get_scenario() const
	{
		return this->scenario;
	}

	Q_INVOKABLE QCoro::QmlTask create_random_map(const QSize &map_size, metternich::era *era)
	{
		return this->create_random_map_coro(map_size, era);
	}

	QCoro::Task<void> create_random_map_coro(const QSize map_size, metternich::era *era);

	Q_INVOKABLE QCoro::QmlTask setup_scenario(metternich::scenario *scenario)
	{
		return this->setup_scenario_coro(scenario);
	}

	QCoro::Task<void> setup_scenario_coro(metternich::scenario *scenario);

	Q_INVOKABLE QCoro::QmlTask start()
	{
		return this->start_coro();
	}

	QCoro::Task<void> start_coro();
	Q_INVOKABLE void stop();

	void clear();
	void reset_game_data();

	void apply_history(const metternich::scenario *scenario);
	void apply_sites();
	void apply_site_buildings(const site *site);
	void apply_population_history();
	int64_t apply_historical_population_group_to_settlement(const population_group_key &group_key, const int population, const site *settlement);

	QCoro::Task<void> on_setup_finished();
	void adjust_food_production_for_country_populations();

	Q_INVOKABLE QCoro::QmlTask do_turn()
	{
		return this->do_turn_coro();
	}

	QCoro::Task<void> do_turn_coro();
	void do_trade();

	const QDate &get_date() const
	{
		return this->date;
	}

	int get_year() const
	{
		return this->get_date().year();
	}

	QDate get_next_date() const;

	int get_turn() const
	{
		return this->turn;
	}

	void increment_turn();

	std::string get_date_string() const;

	QString get_date_qstring() const
	{
		return QString::fromStdString(this->get_date_string());
	}

	const std::vector<country *> &get_countries() const
	{
		return this->countries;
	}

	QVariantList get_countries_qvariant_list() const;
	void add_country(country *country);
	void remove_country(country *country);

	const std::vector<const country *> &get_great_powers() const
	{
		return this->great_powers;
	}

	QVariantList get_great_powers_qvariant_list() const;

	void calculate_great_power_ranks();

	const country *get_player_country() const
	{
		return this->player_country;
	}

	void set_player_country(const country *country)
	{
		if (country == this->get_player_country()) {
			return;
		}

		this->player_country = country;
		emit player_country_changed();
	}

	Q_INVOKABLE int get_price(const metternich::commodity *commodity) const;
	void set_price(const commodity *commodity, const int value);

	void change_price(const commodity *commodity, const int value)
	{
		this->set_price(commodity, this->get_price(commodity) + value);
	}

	[[nodiscard]]
	QCoro::Task<void> create_diplomatic_map_image();

	const QImage &get_exploration_diplomatic_map_image() const
	{
		return this->exploration_diplomatic_map_image;
	}

	[[nodiscard]]
	QCoro::Task<void> create_exploration_diplomatic_map_image();

	void set_exploration_changed()
	{
		this->exploration_changed = true;
	}

	const country *get_wonder_country(const wonder *wonder) const
	{
		const auto find_iterator = this->wonder_countries.find(wonder);

		if (find_iterator != this->wonder_countries.end()) {
			return find_iterator->second;
		}

		return nullptr;
	}

	void set_wonder_country(const wonder *wonder, const country *country)
	{
		if (country == this->get_wonder_country(wonder)) {
			return;
		}

		if (country == nullptr) {
			this->wonder_countries.erase(wonder);
		} else {
			this->wonder_countries[wonder] = country;
		}
	}

	void process_delayed_effects();

private:
	template <typename scope_type>
	void process_delayed_effects(std::vector<std::unique_ptr<delayed_effect_instance<scope_type>>> &delayed_effects)
	{
		for (size_t i = 0; i < delayed_effects.size();) {
			const std::unique_ptr<delayed_effect_instance<scope_type>> &delayed_effect = delayed_effects[i];
			delayed_effect->decrement_remaining_turns();

			if (delayed_effect->get_remaining_turns() <= 0) {
				delayed_effect->do_effects();
				delayed_effects.erase(delayed_effects.begin() + i);
			} else {
				++i;
			}
		}
	}

public:
	void add_delayed_effect(std::unique_ptr<delayed_effect_instance<const character>> &&delayed_effect);
	void add_delayed_effect(std::unique_ptr<delayed_effect_instance<const country>> &&delayed_effect);
	void add_delayed_effect(std::unique_ptr<delayed_effect_instance<const province>> &&delayed_effect);

	void clear_delayed_effects();

	bool do_battle(army *attacking_army, army *defending_army);

signals:
	void running_changed();
	void setup_finished();
	void turn_changed();
	void countries_changed();
	void player_country_changed();

private:
	qunique_ptr<game_rules> rules;
	bool running = false;
	const metternich::scenario *scenario = nullptr;
	QDate date; //the current date in the game
	int turn = 1;
	std::vector<country *> countries; //the countries currently in the game, i.e. those with at least 1 province
	std::vector<const country *> great_powers;
	const country *player_country = nullptr;
	commodity_map<int> prices;
	QImage exploration_diplomatic_map_image;
	bool exploration_changed = false;
	std::map<const wonder *, const country *> wonder_countries;
	std::vector<std::unique_ptr<delayed_effect_instance<const character>>> character_delayed_effects;
	std::vector<std::unique_ptr<delayed_effect_instance<const country>>> country_delayed_effects;
	std::vector<std::unique_ptr<delayed_effect_instance<const province>>> province_delayed_effects;
};

}
