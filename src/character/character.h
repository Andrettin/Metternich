#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "util/fractional_int.h"
#include "util/qunique_ptr.h"

namespace archimedes {
	class calendar;
	enum class gender;
}

namespace metternich {

class advisor_type;
class character_game_data;
class character_history;
class country;
class culture;
class dynasty;
class phenotype;
class portrait;
class province;
class religion;
class site;
class technology;
class trait;
enum class military_unit_category;

template <typename scope_type>
class condition;

template <typename scope_type>
class effect_list;

template <typename scope_type>
class modifier;

class character final : public named_data_entry, public data_type<character>
{
	Q_OBJECT

	Q_PROPERTY(metternich::dynasty* dynasty MEMBER dynasty NOTIFY changed)
	Q_PROPERTY(QString surname READ get_surname_qstring NOTIFY changed)
	Q_PROPERTY(std::string nickname MEMBER nickname NOTIFY changed)
	Q_PROPERTY(std::string epithet MEMBER epithet NOTIFY changed)
	Q_PROPERTY(QString full_name READ get_full_name_qstring NOTIFY changed)
	Q_PROPERTY(QString description READ get_description_qstring NOTIFY changed)
	Q_PROPERTY(metternich::advisor_type* advisor_type MEMBER advisor_type NOTIFY changed)
	Q_PROPERTY(metternich::military_unit_category military_unit_category MEMBER military_unit_category READ get_military_unit_category NOTIFY changed)
	Q_PROPERTY(metternich::culture* culture MEMBER culture NOTIFY changed)
	Q_PROPERTY(metternich::religion* religion MEMBER religion NOTIFY changed)
	Q_PROPERTY(metternich::phenotype* phenotype MEMBER phenotype NOTIFY changed)
	Q_PROPERTY(metternich::portrait* portrait MEMBER portrait NOTIFY changed)
	Q_PROPERTY(metternich::province* home_province MEMBER home_province NOTIFY changed)
	Q_PROPERTY(metternich::site* home_site MEMBER home_site NOTIFY changed)
	Q_PROPERTY(archimedes::gender gender MEMBER gender NOTIFY changed)
	Q_PROPERTY(metternich::character* father MEMBER father NOTIFY changed)
	Q_PROPERTY(metternich::character* mother MEMBER mother NOTIFY changed)
	Q_PROPERTY(QDateTime start_date MEMBER start_date READ get_start_date NOTIFY changed)
	Q_PROPERTY(QDateTime end_date MEMBER end_date READ get_end_date NOTIFY changed)
	Q_PROPERTY(QDateTime birth_date MEMBER birth_date READ get_birth_date NOTIFY changed)
	Q_PROPERTY(QDateTime death_date MEMBER death_date READ get_death_date NOTIFY changed)
	Q_PROPERTY(archimedes::calendar* vital_date_calendar MEMBER vital_date_calendar)
	Q_PROPERTY(int skill MEMBER skill READ get_skill NOTIFY changed)
	Q_PROPERTY(archimedes::centesimal_int skill_multiplier READ get_skill_multiplier WRITE set_skill_multiplier NOTIFY changed)
	Q_PROPERTY(metternich::technology* required_technology MEMBER required_technology NOTIFY changed)
	Q_PROPERTY(metternich::technology* obsolescence_technology MEMBER obsolescence_technology NOTIFY changed)
	Q_PROPERTY(QString leader_type_name READ get_leader_type_name_qstring NOTIFY changed)
	Q_PROPERTY(metternich::character_game_data* game_data READ get_game_data NOTIFY game_data_changed)

public:
	static constexpr const char class_identifier[] = "character";
	static constexpr const char property_class_identifier[] = "metternich::character*";
	static constexpr const char database_folder[] = "characters";
	static constexpr bool history_enabled = true;

	static const std::set<std::string> database_dependencies;

	static constexpr size_t ruler_trait_count = 2;
	static constexpr int max_skill = 6;

	static bool skill_compare(const character *lhs, const character *rhs);

	explicit character(const std::string &identifier);
	~character();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;
	virtual void check() const override;
	virtual data_entry_history *get_history_base() override;

	character_history *get_history() const
	{
		return this->history.get();
	}

	virtual void reset_history() override;

	void reset_game_data();

	character_game_data *get_game_data() const
	{
		return this->game_data.get();
	}

	virtual std::string get_scope_name() const override
	{
		return this->get_full_name();
	}

	const dynasty *get_dynasty() const
	{
		return this->dynasty;
	}

	const std::string &get_surname() const
	{
		return this->surname;
	}

	Q_INVOKABLE void set_surname(const std::string &surname)
	{
		this->surname = surname;
	}

	QString get_surname_qstring() const
	{
		return QString::fromStdString(this->get_surname());
	}

	const std::string &get_nickname() const
	{
		return this->nickname;
	}

	const std::string &get_epithet() const
	{
		return this->epithet;
	}

	std::string get_full_name() const;

	QString get_full_name_qstring() const
	{
		return QString::fromStdString(this->get_full_name());
	}

	const std::string &get_description() const
	{
		return this->description;
	}

	Q_INVOKABLE void set_description(const std::string &description)
	{
		this->description = description;
	}

	QString get_description_qstring() const
	{
		return QString::fromStdString(this->get_description());
	}

	const metternich::advisor_type *get_advisor_type() const
	{
		return this->advisor_type;
	}

	metternich::military_unit_category get_military_unit_category() const
	{
		return this->military_unit_category;
	}

	const metternich::culture *get_culture() const
	{
		return this->culture;
	}

	const metternich::religion *get_religion() const
	{
		return this->religion;
	}

	const metternich::phenotype *get_phenotype() const
	{
		return this->phenotype;
	}

	const metternich::portrait *get_portrait() const
	{
		return this->portrait;
	}

	const province *get_home_province() const
	{
		return this->home_province;
	}

	const site *get_home_site() const
	{
		return this->home_site;
	}

	gender get_gender() const
	{
		return this->gender;
	}

	const character *get_father() const
	{
		return this->father;
	}

	const character *get_mother() const
	{
		return this->mother;
	}

	const QDateTime &get_start_date() const
	{
		return this->start_date;
	}

	const QDateTime &get_end_date() const
	{
		return this->end_date;
	}

	const QDateTime &get_birth_date() const
	{
		return this->birth_date;
	}

	const QDateTime &get_death_date() const
	{
		return this->death_date;
	}

	int get_skill() const
	{
		return this->skill;
	}

	centesimal_int get_skill_multiplier() const
	{
		return centesimal_int(this->get_skill()) / character::max_skill;
	}

	void set_skill_multiplier(const centesimal_int &skill_multiplier)
	{
		this->skill = (skill_multiplier * character::max_skill).to_int();
	}

	const std::vector<const trait *> &get_traits() const
	{
		return this->traits;
	}

	const technology *get_required_technology() const
	{
		return this->required_technology;
	}

	const technology *get_obsolescence_technology() const
	{
		return this->obsolescence_technology;
	}

	const condition<country> *get_conditions() const
	{
		return this->conditions.get();
	}

	const std::vector<const country *> &get_rulable_countries() const
	{
		return this->rulable_countries;
	}

	void add_rulable_country(country *country);

	bool is_ruler() const
	{
		return !this->get_rulable_countries().empty();
	}

	std::string get_ruler_modifier_string(const country *country) const;

	Q_INVOKABLE QString get_ruler_modifier_qstring(metternich::country *country) const
	{
		return QString::fromStdString(this->get_ruler_modifier_string(country));
	}

	Q_INVOKABLE QString get_advisor_effects_string(metternich::country *country) const;

	const effect_list<const country> *get_advisor_effects() const
	{
		return this->advisor_effects.get();
	}

	void apply_advisor_modifier(const country *country, const int multiplier) const;

	bool is_advisor() const
	{
		return this->get_advisor_type() != nullptr;
	}

	int get_advisor_score() const;

	bool is_leader() const;
	bool is_admiral() const;
	std::string get_leader_type_name() const;

	QString get_leader_type_name_qstring() const
	{
		return QString::fromStdString(this->get_leader_type_name());
	}

signals:
	void changed();
	void game_data_changed() const;

private:
	metternich::dynasty *dynasty = nullptr;
	std::string surname;
	std::string nickname;
	std::string epithet;
	std::string description;
	metternich::advisor_type *advisor_type = nullptr;
	metternich::military_unit_category military_unit_category;
	metternich::culture *culture = nullptr;
	metternich::religion *religion = nullptr;
	metternich::phenotype *phenotype = nullptr;
	metternich::portrait *portrait = nullptr;
	province *home_province = nullptr;
	site *home_site = nullptr;
	archimedes::gender gender;
	character *father = nullptr;
	character *mother = nullptr;
	QDateTime start_date;
	QDateTime end_date;
	QDateTime birth_date;
	QDateTime death_date;
	calendar *vital_date_calendar = nullptr; //the calendar for the birth, death, start and end dates
	int skill = 1;
	std::vector<const country *> rulable_countries;
	std::vector<const trait *> traits;
	technology *required_technology = nullptr;
	technology *obsolescence_technology = nullptr;
	std::unique_ptr<const condition<country>> conditions;
	std::unique_ptr<const modifier<const country>> advisor_modifier;
	std::unique_ptr<const effect_list<const country>> advisor_effects;
	qunique_ptr<character_history> history;
	qunique_ptr<character_game_data> game_data;
};

}
