#pragma once

#include "domain/consulate_container.h"
#include "domain/domain_container.h"
#include "script/opinion_modifier_container.h"

Q_MOC_INCLUDE("domain/domain.h")
Q_MOC_INCLUDE("domain/subject_type.h")

namespace metternich {

class domain;
class domain_game_data;
class subject_type;
enum class diplomacy_state;
enum class diplomatic_map_mode;

class domain_diplomacy final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(const metternich::domain* overlord READ get_overlord NOTIFY overlord_changed)
	Q_PROPERTY(const metternich::subject_type* subject_type READ get_subject_type NOTIFY subject_type_changed)
	Q_PROPERTY(QVariantList vassals READ get_vassals_qvariant_list NOTIFY diplomacy_states_changed)
	Q_PROPERTY(QVariantList subject_type_counts READ get_subject_type_counts_qvariant_list NOTIFY diplomacy_states_changed)
	Q_PROPERTY(QVariantList consulates READ get_consulates_qvariant_list NOTIFY consulates_changed)
	Q_PROPERTY(QRect diplomatic_map_image_rect READ get_diplomatic_map_image_rect NOTIFY diplomatic_map_image_changed)
	Q_PROPERTY(QRect realm_diplomatic_map_image_rect READ get_realm_diplomatic_map_image_rect NOTIFY realm_diplomatic_map_image_changed)
	Q_PROPERTY(QColor diplomatic_map_color READ get_diplomatic_map_color NOTIFY overlord_changed)

public:
	explicit domain_diplomacy(const metternich::domain *domain, const domain_game_data *game_data);
	~domain_diplomacy();

	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);

	gsml_data to_gsml_data() const;

	domain_game_data *get_game_data() const;

	[[nodiscard]] QCoro::Task<void> apply_diplomatic_history();

	const metternich::domain *get_overlord() const
	{
		return this->overlord;
	}

	[[nodiscard]] QCoro::Task<void> set_overlord(const metternich::domain *overlord);

	bool is_vassal_of(const metternich::domain *domain) const;
	bool is_any_vassal_of(const metternich::domain *domain) const;

	Q_INVOKABLE bool is_any_vassal_of(metternich::domain *domain)
	{
		const metternich::domain *domain_const = domain;
		return this->is_any_vassal_of(domain_const);
	}

	bool is_overlord_of(const metternich::domain *domain) const;
	bool is_any_overlord_of(const metternich::domain *domain) const;

	Q_INVOKABLE bool is_independent() const
	{
		return this->get_overlord() == nullptr;
	}

	const metternich::subject_type *get_subject_type() const
	{
		return this->subject_type;
	}

	[[nodiscard]] QCoro::Task<void> set_subject_type(const metternich::subject_type *subject_type);

	const domain_set &get_known_countries() const
	{
		return this->known_countries;
	}

	bool is_country_known(const metternich::domain *other_domain) const
	{
		return this->get_known_countries().contains(other_domain);
	}

	[[nodiscard]] QCoro::Task<void> add_known_country(const metternich::domain *other_domain);

	void remove_known_country(const metternich::domain *other_domain)
	{
		this->known_countries.erase(other_domain);
	}

	diplomacy_state get_diplomacy_state(const metternich::domain *other_domain) const;
	[[nodiscard]] QCoro::Task<void> set_diplomacy_state(const metternich::domain *other_domain, const diplomacy_state state);

	const std::map<diplomacy_state, int> &get_diplomacy_state_counts() const
	{
		return this->diplomacy_state_counts;
	}

	void change_diplomacy_state_count(const diplomacy_state state, const int change);
	Q_INVOKABLE QString get_diplomacy_state_diplomatic_map_suffix(metternich::domain *other_domain) const;

	bool at_war() const;

	bool can_attack(const metternich::domain *other_domain) const;

	std::optional<diplomacy_state> get_offered_diplomacy_state(const metternich::domain *other_domain) const;

	Q_INVOKABLE int get_offered_diplomacy_state_int(metternich::domain *other_domain) const
	{
		const std::optional<diplomacy_state> state = this->get_offered_diplomacy_state(other_domain);

		if (!state.has_value()) {
			return -1;
		}

		return static_cast<int>(state.value());
	}

	void set_offered_diplomacy_state(const metternich::domain *other_domain, const std::optional<diplomacy_state> &state);

	Q_INVOKABLE void set_offered_diplomacy_state_int(metternich::domain *other_domain, const int state)
	{
		if (state == -1) {
			this->set_offered_diplomacy_state(other_domain, std::nullopt);
		} else {
			this->set_offered_diplomacy_state(other_domain, static_cast<diplomacy_state>(state));
		}
	}

	QVariantList get_consulates_qvariant_list() const;

	const consulate *get_consulate(const metternich::domain *other_domain) const
	{
		const auto find_iterator = this->consulates.find(other_domain);

		if (find_iterator != this->consulates.end()) {
			return find_iterator->second;
		}

		return nullptr;
	}

	void set_consulate(const metternich::domain *other_domain, const consulate *consulate);

	int get_opinion_of(const metternich::domain *other) const;

	int get_base_opinion(const metternich::domain *other) const
	{
		const auto find_iterator = this->base_opinions.find(other);
		if (find_iterator != this->base_opinions.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_base_opinion(const metternich::domain *other, const int opinion);

	void change_base_opinion(const metternich::domain *other, const int change)
	{
		this->set_base_opinion(other, this->get_base_opinion(other) + change);
	}

	const opinion_modifier_map<int> &get_opinion_modifiers_for(const metternich::domain *other) const
	{
		static const opinion_modifier_map<int> empty_map;

		const auto find_iterator = this->opinion_modifiers.find(other);
		if (find_iterator != this->opinion_modifiers.end()) {
			return find_iterator->second;
		}

		return empty_map;
	}

	void add_opinion_modifier(const metternich::domain *other, const opinion_modifier *modifier, const int duration);
	void remove_opinion_modifier(const metternich::domain *other, const opinion_modifier *modifier);
	void decrement_opinion_modifiers();

	std::vector<const metternich::domain *> get_vassals() const;
	QVariantList get_vassals_qvariant_list() const;
	QVariantList get_subject_type_counts_qvariant_list() const;

	const QColor &get_diplomatic_map_color() const;

	const QPromise<QImage> *get_diplomatic_map_image_promise() const
	{
		return this->diplomatic_map_image_promise.get();
	}

	QImage prepare_diplomatic_map_image() const;
	[[nodiscard]] static QImage finalize_diplomatic_map_image(QImage &&image);

	void create_diplomatic_map_image();

	const QRect &get_diplomatic_map_image_rect() const
	{
		return this->diplomatic_map_image_rect;
	}

	const QPromise<QImage> *get_selected_diplomatic_map_image_promise() const
	{
		return this->selected_diplomatic_map_image_promise.get();
	}

	const QPromise<QImage> *get_realm_diplomatic_map_image_promise() const
	{
		return this->realm_diplomatic_map_image_promise.get();
	}

	QImage prepare_realm_diplomatic_map_image() const;

	void create_realm_diplomatic_map_image();

	const QRect &get_realm_diplomatic_map_image_rect() const
	{
		return this->realm_diplomatic_map_image_rect;
	}

	const QPromise<QImage> *get_selected_realm_diplomatic_map_image_promise() const
	{
		return this->selected_realm_diplomatic_map_image_promise.get();
	}

	const QPromise<QImage> *get_diplomatic_map_mode_image_promise(const diplomatic_map_mode mode) const
	{
		const auto find_iterator = this->diplomatic_map_mode_image_promises.find(mode);
		if (find_iterator != this->diplomatic_map_mode_image_promises.end()) {
			return find_iterator->second.get();
		}

		throw std::runtime_error(std::format("No diplomatic map image promise found for mode {}.", static_cast<int>(mode)));
	}

	void create_diplomatic_map_mode_image(const diplomatic_map_mode mode);

	const QPromise<QImage> *get_diplomacy_state_diplomatic_map_image_promise(const diplomacy_state state) const
	{
		const auto find_iterator = this->diplomacy_state_diplomatic_map_image_promises.find(state);
		if (find_iterator != this->diplomacy_state_diplomatic_map_image_promises.end()) {
			return find_iterator->second.get();
		}

		throw std::runtime_error(std::format("No diplomacy state diplomatic map image found for state {}.", static_cast<int>(state)));
	}

	void create_diplomacy_state_diplomatic_map_image(const diplomacy_state state);

	bool can_declare_war_on(const metternich::domain *other_domain) const;

	int get_diplomatic_penalty_for_expansion_modifier() const
	{
		return this->diplomatic_penalty_for_expansion_modifier;
	}

	void change_diplomatic_penalty_for_expansion_modifier(const int change)
	{
		this->diplomatic_penalty_for_expansion_modifier += change;
	}

	int get_free_consulate_count(const consulate *consulate) const
	{
		const auto find_iterator = this->free_consulate_counts.find(consulate);

		if (find_iterator != this->free_consulate_counts.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_free_consulate_count(const consulate *consulate, const int value);

	void change_free_consulate_count(const consulate *consulate, const int value)
	{
		this->set_free_consulate_count(consulate, this->get_free_consulate_count(consulate) + value);
	}

signals:
	void overlord_changed();
	void subject_type_changed();
	void diplomacy_states_changed();
	void offered_diplomacy_states_changed();
	void consulates_changed();
	void diplomatic_map_image_changed();
	void realm_diplomatic_map_image_changed();

private:
	const metternich::domain *domain = nullptr;
	const metternich::domain *overlord = nullptr;
	const metternich::subject_type *subject_type = nullptr;
	domain_set known_countries;
	domain_map<diplomacy_state> diplomacy_states;
	std::map<diplomacy_state, int> diplomacy_state_counts;
	domain_map<diplomacy_state> offered_diplomacy_states;
	domain_map<const consulate *> consulates;
	domain_map<int> base_opinions;
	domain_map<opinion_modifier_map<int>> opinion_modifiers;
	std::shared_ptr<QPromise<QImage>> diplomatic_map_image_promise;
	std::shared_ptr<QPromise<QImage>> selected_diplomatic_map_image_promise;
	std::shared_ptr<QPromise<QImage>> realm_diplomatic_map_image_promise;
	std::shared_ptr<QPromise<QImage>> selected_realm_diplomatic_map_image_promise;
	std::map<diplomatic_map_mode, std::shared_ptr<QPromise<QImage>>> diplomatic_map_mode_image_promises;
	std::map<diplomacy_state, std::shared_ptr<QPromise<QImage>>> diplomacy_state_diplomatic_map_image_promises;
	QRect diplomatic_map_image_rect;
	QRect realm_diplomatic_map_image_rect;
	int diplomatic_penalty_for_expansion_modifier = 0;
	consulate_map<int> free_consulate_counts;
};

}
