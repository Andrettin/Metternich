#pragma once

#include "character/character_base.h"
#include "database/data_entry_container.h"
#include "database/data_type.h"
#include "util/centesimal_int.h"
#include "util/qunique_ptr.h"

Q_MOC_INCLUDE("character/bloodline.h")
Q_MOC_INCLUDE("character/character_class.h")
Q_MOC_INCLUDE("character/character_game_data.h")
Q_MOC_INCLUDE("character/dynasty.h")
Q_MOC_INCLUDE("character/monster_type.h")
Q_MOC_INCLUDE("culture/culture.h")
Q_MOC_INCLUDE("map/province.h")
Q_MOC_INCLUDE("map/site.h")
Q_MOC_INCLUDE("religion/deity.h")
Q_MOC_INCLUDE("religion/religion.h")
Q_MOC_INCLUDE("species/phenotype.h")
Q_MOC_INCLUDE("species/species.h")
Q_MOC_INCLUDE("ui/portrait.h")

namespace archimedes {
	class calendar;
	class word;
	enum class gender;
}

namespace metternich {

class bloodline;
class character_attribute;
class character_class;
class character_game_data;
class character_history;
class character_reference;
class civilian_unit_class;
class civilian_unit_type;
class culture;
class deity;
class domain;
class dynasty;
class item_type;
class monster_type;
class phenotype;
class portrait;
class province;
class religion;
class site;
class species;
class trait;
enum class bloodline_strength_category;
enum class military_unit_category;

template <typename scope_type>
class and_condition;

template <typename scope_type>
class modifier;

class character final : public character_base, public data_type<character>
{
	Q_OBJECT

	Q_PROPERTY(archimedes::word* name_front_compound_element MEMBER name_front_compound_element WRITE set_name_front_compound_element NOTIFY changed)
	Q_PROPERTY(archimedes::word* name_rear_compound_element MEMBER name_rear_compound_element WRITE set_name_rear_compound_element NOTIFY changed)
	Q_PROPERTY(metternich::dynasty* dynasty MEMBER dynasty NOTIFY changed)
	Q_PROPERTY(metternich::species* species MEMBER species NOTIFY changed)
	Q_PROPERTY(const metternich::character_class* character_class MEMBER character_class READ get_character_class NOTIFY changed)
	Q_PROPERTY(int level MEMBER level READ get_level NOTIFY changed)
	Q_PROPERTY(std::string rank MEMBER rank NOTIFY changed)
	Q_PROPERTY(const metternich::monster_type* monster_type MEMBER monster_type READ get_monster_type NOTIFY changed)
	Q_PROPERTY(metternich::culture* culture MEMBER culture NOTIFY changed)
	Q_PROPERTY(const metternich::religion* religion MEMBER religion NOTIFY changed)
	Q_PROPERTY(const metternich::phenotype* phenotype MEMBER phenotype READ get_phenotype NOTIFY changed)
	Q_PROPERTY(const metternich::deity* deity READ get_deity CONSTANT)
	Q_PROPERTY(const metternich::bloodline* bloodline MEMBER bloodline READ get_bloodline NOTIFY changed)
	Q_PROPERTY(int bloodline_strength MEMBER bloodline_strength READ get_bloodline_strength NOTIFY changed)
	Q_PROPERTY(metternich::bloodline_strength_category bloodline_strength_category MEMBER bloodline_strength_category NOTIFY changed)
	Q_PROPERTY(bool generated_bloodline MEMBER generated_bloodline NOTIFY changed)
	Q_PROPERTY(metternich::portrait* portrait MEMBER portrait NOTIFY changed)
	Q_PROPERTY(const metternich::site* home_site MEMBER home_site READ get_home_site NOTIFY changed)
	Q_PROPERTY(metternich::character* father READ get_father WRITE set_father NOTIFY changed)
	Q_PROPERTY(metternich::character* mother READ get_mother WRITE set_mother NOTIFY changed)
	Q_PROPERTY(metternich::character* contemporary_character READ get_contemporary_character WRITE set_contemporary_character NOTIFY changed)
	Q_PROPERTY(int skill MEMBER skill READ get_skill NOTIFY changed)
	Q_PROPERTY(archimedes::centesimal_int skill_multiplier READ get_skill_multiplier WRITE set_skill_multiplier NOTIFY changed)
	Q_PROPERTY(int hit_points MEMBER hit_points READ get_hit_points NOTIFY changed)
	Q_PROPERTY(QString leader_type_name READ get_leader_type_name_qstring NOTIFY changed)
	Q_PROPERTY(metternich::character_game_data* game_data READ get_game_data NOTIFY game_data_changed)
	Q_PROPERTY(bool temporary READ is_temporary CONSTANT)
	Q_PROPERTY(const QObject* tree_parent READ get_tree_parent CONSTANT)
	Q_PROPERTY(QVariantList secondary_tree_parents READ get_secondary_tree_parents CONSTANT)

public:
	static constexpr const char class_identifier[] = "character";
	static constexpr const char property_class_identifier[] = "metternich::character*";
	static constexpr const char database_folder[] = "characters";
	static constexpr bool history_enabled = true;

	static const std::set<std::string> database_dependencies;

	static void initialize_all();
	static void initialize_all_vital_dates();
	static void initialize_all_home_sites();
	static void initialize_all_bloodlines();

	static bool skill_compare(const character *lhs, const character *rhs);

	static character *generate(const metternich::species *species, const metternich::character_class *character_class, const int level, const metternich::monster_type *monster_type, const metternich::culture *culture, const metternich::religion *religion, const site *home_site, const std::vector<const trait *> &traits, const int hit_points, const std::vector<const item_type *> &items, const bool generate_bloodline, const bool temporary = false);
	static character *generate(const metternich::monster_type *monster_type, const metternich::culture *culture, const metternich::religion *religion, const site *home_site, const int hit_points, const std::vector<const item_type *> &items, const bool generate_bloodline, const bool temporary = false);
	static std::shared_ptr<character_reference> generate_temporary(const metternich::monster_type *monster_type, const metternich::culture *culture, const metternich::religion *religion, const site *home_site, const int hit_points, const std::vector<const item_type *> &items);

	explicit character(const std::string &identifier);
	~character();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;
	virtual void check() const override;
	virtual data_entry_history *get_history_base() override;

	gsml_data to_gsml_data() const;

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

	bool initialize_home_site_from_children();
	bool initialize_home_site_from_parents();

	void initialize_bloodline();
	void initialize_bloodline_from_parents();

	void set_name_front_compound_element(word *word);
	void set_name_rear_compound_element(word *word);

	const dynasty *get_dynasty() const
	{
		return this->dynasty;
	}

	virtual bool is_surname_first() const override;

	const metternich::species *get_species() const
	{
		return this->species;
	}

	const metternich::character_class *get_character_class() const
	{
		return this->character_class;
	}

	const military_unit_category get_military_unit_category() const;
	const civilian_unit_class *get_civilian_unit_class() const;
	const civilian_unit_type *get_civilian_unit_type() const;

	int get_level() const
	{
		return this->level;
	}

	const metternich::monster_type *get_monster_type() const
	{
		return this->monster_type;
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

	const metternich::deity *get_deity() const
	{
		return this->deity;
	}

	void set_deity(const metternich::deity *deity)
	{
		this->deity = deity;
	}

	bool is_innate_deity() const;

	const metternich::bloodline *get_bloodline() const
	{
		return this->bloodline;
	}

	void set_bloodline(const metternich::bloodline *bloodline)
	{
		if (bloodline == this->get_bloodline()) {
			return;
		}

		this->bloodline = bloodline;
	}

	int get_bloodline_strength() const
	{
		return this->bloodline_strength;
	}

	void set_bloodline_strength(const int bloodline_strength)
	{
		if (bloodline_strength == this->get_bloodline_strength()) {
			return;
		}

		this->bloodline_strength = bloodline_strength;
	}

	void generate_bloodline();

	virtual int get_adulthood_age() const override;
	virtual int get_venerable_age() const override;
	virtual const dice &get_maximum_age_modifier() const override;
	virtual const dice &get_starting_age_modifier() const override;

	virtual bool is_immortal() const override;

	const metternich::portrait *get_portrait() const
	{
		return this->portrait;
	}

	const site *get_home_site() const
	{
		return this->home_site;
	}

	character *get_father() const
	{
		return static_cast<character *>(character_base::get_father());
	}

	character *get_mother() const
	{
		return static_cast<character *>(character_base::get_mother());
	}

	std::vector<character *> get_parents() const
	{
		std::vector<character *> parents;
		if (this->get_father() != nullptr) {
			parents.push_back(this->get_father());
		}
		if (this->get_mother() != nullptr) {
			parents.push_back(this->get_mother());
		}
		return parents;
	}

	character *get_contemporary_character() const
	{
		return static_cast<character *>(character_base::get_contemporary_character());
	}

	const character_attribute *get_primary_attribute() const;

	int get_skill() const
	{
		return this->skill;
	}

	centesimal_int get_skill_multiplier() const;
	void set_skill_multiplier(const centesimal_int &skill_multiplier);

	std::optional<std::pair<int, int>> get_attribute_range(const character_attribute *attribute) const;

	int get_hit_points() const
	{
		return this->hit_points;
	}

	const std::vector<const trait *> &get_traits() const
	{
		return this->traits;
	}

	const std::vector<const item_type *> &get_starting_items() const
	{
		return this->starting_items;
	}

	const and_condition<domain> *get_conditions() const
	{
		return this->conditions.get();
	}

	bool is_admiral() const;
	bool is_explorer() const;
	std::string_view get_leader_type_name() const;

	QString get_leader_type_name_qstring() const
	{
		return QString::fromStdString(std::string(this->get_leader_type_name()));
	}

	bool is_temporary() const
	{
		return this->temporary;
	}

	virtual named_data_entry *get_tree_parent() const override;
	QVariantList get_secondary_tree_parents() const;

	virtual int get_tree_y() const override
	{
		if (this->get_tree_parent() != nullptr) {
			return this->get_tree_parent()->get_tree_y() + 1;
		}

		return 0;
	}

	virtual std::vector<const named_data_entry *> get_top_tree_elements() const override;
	virtual bool is_hidden_in_tree() const override;
	Q_INVOKABLE QVariantList get_tree_characters() const;

signals:
	void changed();
	void game_data_changed() const;

private:
	word *name_front_compound_element = nullptr;
	word *name_rear_compound_element = nullptr;
	metternich::dynasty *dynasty = nullptr;
	metternich::species *species = nullptr;
	const metternich::character_class *character_class = nullptr;
	int level = 0;
	std::string rank;
	const metternich::monster_type *monster_type = nullptr;
	metternich::culture *culture = nullptr;
	const metternich::religion *religion = nullptr;
	const metternich::phenotype *phenotype = nullptr;
	const metternich::deity *deity = nullptr; //the deity which the character is (if it is a deity)
	const metternich::bloodline *bloodline = nullptr;
	int bloodline_strength = 0;
	metternich::bloodline_strength_category bloodline_strength_category {};
	bool generated_bloodline = false;
	bool bloodline_initialized = false;
	metternich::portrait *portrait = nullptr;
	const site *home_site = nullptr;
	int skill = 0;
	data_entry_map<character_attribute, std::string> attribute_ratings;
	data_entry_map<character_attribute, std::pair<int, int>> attribute_ranges;
	int hit_points = 0;
	std::vector<const trait *> traits;
	std::vector<const item_type *> starting_items;
	std::unique_ptr<const and_condition<domain>> conditions;
	qunique_ptr<character_history> history;
	qunique_ptr<character_game_data> game_data;
	bool temporary = false;
	const named_data_entry *top_tree_element = nullptr;
};

}
