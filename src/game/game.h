#pragma once

#include "economy/commodity_container.h"
#include "util/qunique_ptr.h"
#include "util/singleton.h"

Q_MOC_INCLUDE("domain/domain.h")

namespace archimedes {
	class era;
	class gsml_data;
	class gsml_property;
}

namespace metternich {

class army;
class character;
class combat;
class domain;
class event;
class game_rules;
class party;
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
	Q_PROPERTY(const metternich::character* player_character READ get_player_character WRITE set_player_character NOTIFY player_character_changed)
	Q_PROPERTY(const metternich::domain* player_country READ get_player_country WRITE set_player_country NOTIFY player_country_changed)
	Q_PROPERTY(bool combat_running READ is_combat_running NOTIFY combat_running_changed)
	Q_PROPERTY(metternich::combat* current_combat READ get_current_combat NOTIFY current_combat_changed)
	Q_PROPERTY(const metternich::game_rules* rules READ get_rules CONSTANT)

public:
	static QDate normalize_date(const QDate &date);

	game();
	~game();

	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);

	gsml_data to_gsml_data() const;

	void save(const std::filesystem::path &filepath) const;
	Q_INVOKABLE void save(const QUrl &filepath) const;

	[[nodiscard]]
	QCoro::Task<void> load(const std::filesystem::path &filepath);

	Q_INVOKABLE QCoro::QmlTask load(const QUrl &filepath);

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

	Q_INVOKABLE QCoro::QmlTask setup_scenario(metternich::scenario *scenario)
	{
		return this->setup_scenario_coro(scenario);
	}

	[[nodiscard]]
	QCoro::Task<void> setup_scenario_coro(metternich::scenario *scenario);

	Q_INVOKABLE QCoro::QmlTask start()
	{
		return this->start_coro();
	}

	[[nodiscard]]
	QCoro::Task<void> start_coro();

	Q_INVOKABLE void stop();

	void clear();
	void reset_game_data();

	void apply_history(const metternich::scenario *scenario);
	void apply_sites();
	void apply_site_buildings(const site *site);
	void apply_population_history();
	int64_t apply_historical_population_group_to_site(const population_group_key &group_key, const int population, const site *site);
	void apply_historical_population_units_to_site(const population_group_key &group_key, const int population_unit_count, const site *site);

	QCoro::Task<void> on_setup_finished();

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

	int get_current_months_per_turn() const;
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

	const std::vector<domain *> &get_countries() const
	{
		return this->countries;
	}

	QVariantList get_countries_qvariant_list() const;
	void add_country(domain *domain);
	void remove_country(domain *domain);

	void calculate_country_ranks();

	const character *get_player_character() const
	{
		return this->player_character;
	}

	void set_player_character(const character *character)
	{
		if (character == this->get_player_character()) {
			return;
		}

		this->player_character = character;
		emit player_character_changed();
	}

	const domain *get_player_country() const
	{
		return this->player_country;
	}

	void set_player_country(const domain *domain);

	Q_INVOKABLE int get_price(const metternich::commodity *commodity) const;
	void set_price(const commodity *commodity, const int value);

	void change_price(const commodity *commodity, const int value)
	{
		this->set_price(commodity, this->get_price(commodity) + value);
	}

	[[nodiscard]]
	QCoro::Task<void> create_map_images();

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

	const domain *get_wonder_country(const wonder *wonder) const
	{
		const auto find_iterator = this->wonder_countries.find(wonder);

		if (find_iterator != this->wonder_countries.end()) {
			return find_iterator->second;
		}

		return nullptr;
	}

	void set_wonder_country(const wonder *wonder, const domain *country)
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

	const character *get_character(const std::string &identifier) const;

	const std::vector<qunique_ptr<character>> &get_generated_characters() const
	{
		return this->generated_characters;
	}

	void add_generated_character(qunique_ptr<character> &&character);
	void remove_generated_character(character *character);

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
	void add_delayed_effect(std::unique_ptr<delayed_effect_instance<const domain>> &&delayed_effect);
	void add_delayed_effect(std::unique_ptr<delayed_effect_instance<const province>> &&delayed_effect);
	void add_delayed_effect(std::unique_ptr<delayed_effect_instance<const site>> &&delayed_effect);
	void clear_delayed_effects();

	bool has_fired_event(const metternich::event *event) const;
	void add_fired_event(const metternich::event *event);

	bool do_battle(army *attacking_army, army *defending_army);

	combat *get_current_combat() const
	{
		return this->current_combat.get();
	}

	void set_current_combat(qunique_ptr<combat> &&combat);

	bool is_combat_running() const
	{
		return this->get_current_combat() != nullptr;
	}

signals:
	void running_changed();
	void setup_finished();
	void turn_changed();
	void countries_changed();
	void player_character_changed();
	void player_country_changed();
	void combat_running_changed();
	void current_combat_changed();
	void game_over();

private:
	qunique_ptr<game_rules> rules;
	bool running = false;
	const metternich::scenario *scenario = nullptr;
	QDate date; //the current date in the game
	int turn = 1;
	std::vector<domain *> countries; //the countries currently in the game, i.e. those with at least 1 province
	const character *player_character = nullptr;
	const domain *player_country = nullptr;
	commodity_map<int> prices;
	QImage exploration_diplomatic_map_image;
	bool exploration_changed = false;
	std::map<const wonder *, const domain *> wonder_countries;
	std::vector<qunique_ptr<character>> generated_characters;
	std::map<std::string, const character *> generated_characters_by_identifier;
	std::vector<std::unique_ptr<delayed_effect_instance<const character>>> character_delayed_effects;
	std::vector<std::unique_ptr<delayed_effect_instance<const domain>>> country_delayed_effects;
	std::vector<std::unique_ptr<delayed_effect_instance<const province>>> province_delayed_effects;
	std::vector<std::unique_ptr<delayed_effect_instance<const site>>> site_delayed_effects;
	std::set<const metternich::event *> fired_events;
	qunique_ptr<combat> current_combat;
};

}
