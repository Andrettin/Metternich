#pragma once

#include "character/character_base.h"
#include "database/data_type.h"
#include "util/fractional_int.h"
#include "util/qunique_ptr.h"

Q_MOC_INCLUDE("character/character_game_data.h")
Q_MOC_INCLUDE("character/character_type.h")
Q_MOC_INCLUDE("character/dynasty.h")
Q_MOC_INCLUDE("country/culture.h")
Q_MOC_INCLUDE("country/religion.h")
Q_MOC_INCLUDE("map/province.h")
Q_MOC_INCLUDE("map/site.h")
Q_MOC_INCLUDE("species/phenotype.h")
Q_MOC_INCLUDE("species/species.h")
Q_MOC_INCLUDE("technology/technology.h")
Q_MOC_INCLUDE("ui/portrait.h")

namespace archimedes {
	class calendar;
	enum class gender;
}

namespace metternich {

class character_game_data;
class character_history;
class character_type;
class civilian_unit_class;
class civilian_unit_type;
class country;
class culture;
class dynasty;
class phenotype;
class portrait;
class province;
class religion;
class site;
class species;
class technology;
class trait;
enum class character_attribute;
enum class character_role;
enum class military_unit_category;

template <typename scope_type>
class and_condition;

template <typename scope_type>
class effect_list;

template <typename scope_type>
class modifier;

class character final : public character_base, public data_type<character>
{
	Q_OBJECT

	Q_PROPERTY(metternich::dynasty* dynasty MEMBER dynasty NOTIFY changed)
	Q_PROPERTY(metternich::character_role role MEMBER role READ get_role NOTIFY changed)
	Q_PROPERTY(const metternich::character_type* character_type MEMBER character_type READ get_character_type NOTIFY changed)
	Q_PROPERTY(metternich::species* species MEMBER species NOTIFY changed)
	Q_PROPERTY(metternich::culture* culture MEMBER culture NOTIFY changed)
	Q_PROPERTY(metternich::religion* religion MEMBER religion NOTIFY changed)
	Q_PROPERTY(metternich::phenotype* phenotype MEMBER phenotype NOTIFY changed)
	Q_PROPERTY(metternich::portrait* portrait MEMBER portrait NOTIFY changed)
	Q_PROPERTY(const metternich::site* home_settlement MEMBER home_settlement NOTIFY changed)
	Q_PROPERTY(const metternich::site* home_site MEMBER home_site NOTIFY changed)
	Q_PROPERTY(metternich::character* father READ get_father WRITE set_father NOTIFY changed)
	Q_PROPERTY(metternich::character* mother READ get_mother WRITE set_mother NOTIFY changed)
	Q_PROPERTY(int skill MEMBER skill READ get_skill NOTIFY changed)
	Q_PROPERTY(archimedes::centesimal_int skill_multiplier READ get_skill_multiplier WRITE set_skill_multiplier NOTIFY changed)
	Q_PROPERTY(metternich::province* governable_province MEMBER governable_province NOTIFY changed)
	Q_PROPERTY(metternich::site* holdable_site MEMBER holdable_site NOTIFY changed)
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

	static void initialize_all();

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

	bool initialize_dates_from_children();
	bool initialize_dates_from_parents();

	const dynasty *get_dynasty() const
	{
		return this->dynasty;
	}

	virtual bool is_surname_first() const override;

	character_role get_role() const
	{
		return this->role;
	}

	const metternich::character_type *get_character_type() const
	{
		return this->character_type;
	}

	const military_unit_category get_military_unit_category() const;
	const civilian_unit_class *get_civilian_unit_class() const;
	const civilian_unit_type *get_civilian_unit_type() const;

	const metternich::species *get_species() const
	{
		return this->species;
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

	virtual int get_adulthood_age() const override;
	virtual int get_venerable_age() const override;
	virtual const dice &get_maximum_age_modifier() const override;
	virtual const dice &get_starting_age_modifier() const override;

	virtual bool is_immortal() const override
	{
		return false;
	}

	const metternich::portrait *get_portrait() const
	{
		return this->portrait;
	}

	const site *get_home_settlement() const
	{
		return this->home_settlement;
	}

	character *get_father() const
	{
		return static_cast<character *>(character_base::get_father());
	}

	character *get_mother() const
	{
		return static_cast<character *>(character_base::get_mother());
	}

	character_attribute get_primary_attribute() const;

	int get_skill() const
	{
		return this->skill;
	}

	centesimal_int get_skill_multiplier() const;
	void set_skill_multiplier(const centesimal_int &skill_multiplier);

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

	const and_condition<country> *get_conditions() const
	{
		return this->conditions.get();
	}

	const std::vector<const country *> &get_rulable_countries() const
	{
		return this->rulable_countries;
	}

	void add_rulable_country(country *country);

	const province *get_governable_province() const
	{
		return this->governable_province;
	}

	const site *get_holdable_site() const
	{
		return this->holdable_site;
	}

	bool is_admiral() const;
	bool is_explorer() const;
	std::string_view get_leader_type_name() const;

	QString get_leader_type_name_qstring() const
	{
		return QString::fromStdString(std::string(this->get_leader_type_name()));
	}

signals:
	void changed();
	void game_data_changed() const;

private:
	metternich::dynasty *dynasty = nullptr;
	metternich::character_role role;
	const metternich::character_type *character_type = nullptr;
	metternich::species *species = nullptr;
	metternich::culture *culture = nullptr;
	metternich::religion *religion = nullptr;
	metternich::phenotype *phenotype = nullptr;
	metternich::portrait *portrait = nullptr;
	const site *home_settlement = nullptr;
	const site *home_site = nullptr;
	int skill = 0;
	std::vector<const country *> rulable_countries;
	province *governable_province = nullptr;
	site *holdable_site = nullptr;
	std::vector<const trait *> traits;
	technology *required_technology = nullptr;
	technology *obsolescence_technology = nullptr;
	std::unique_ptr<const and_condition<country>> conditions;
	qunique_ptr<character_history> history;
	qunique_ptr<character_game_data> game_data;
};

}
