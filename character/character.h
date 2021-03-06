#pragma once

#include "database/data_entry.h"
#include "database/data_type.h"

#include <QDateTime>
#include <QVariant>

#include <string>
#include <vector>

namespace metternich {

class commodity;
class culture;
class dynasty;
class government_type;
class gsml_property;
class holding;
class item;
class landed_title;
class law;
class phenotype;
class province;
class religion;
class trait;

template <typename T>
class condition_check;

template <typename T>
class scoped_flag;

class character final : public data_entry, public data_type<character>
{
	Q_OBJECT

	Q_PROPERTY(QString name READ get_name_qstring WRITE set_name_qstring NOTIFY name_changed)
	Q_PROPERTY(QString full_name READ get_full_name_qstring NOTIFY full_name_changed)
	Q_PROPERTY(QString titled_name READ get_titled_name_qstring NOTIFY titled_name_changed)
	Q_PROPERTY(bool female MEMBER female READ is_female)
	Q_PROPERTY(metternich::dynasty* dynasty READ get_dynasty WRITE set_dynasty NOTIFY dynasty_changed)
	Q_PROPERTY(metternich::culture* culture READ get_culture WRITE set_culture NOTIFY culture_changed)
	Q_PROPERTY(metternich::religion* religion MEMBER religion READ get_religion NOTIFY religion_changed)
	Q_PROPERTY(metternich::phenotype* phenotype MEMBER phenotype READ get_phenotype)
	Q_PROPERTY(metternich::landed_title* primary_title READ get_primary_title WRITE set_primary_title NOTIFY primary_title_changed)
	Q_PROPERTY(metternich::character* father READ get_father WRITE set_father)
	Q_PROPERTY(metternich::character* mother READ get_mother WRITE set_mother)
	Q_PROPERTY(metternich::character* spouse READ get_spouse WRITE set_spouse)
	Q_PROPERTY(metternich::character* liege READ get_liege WRITE set_liege NOTIFY liege_changed)
	Q_PROPERTY(metternich::character* employer READ get_liege WRITE set_liege NOTIFY liege_changed)
	Q_PROPERTY(metternich::province* capital_province READ get_capital_province NOTIFY capital_province_changed)
	Q_PROPERTY(QVariantList traits READ get_traits_qvariant_list NOTIFY traits_changed)
	Q_PROPERTY(QVariantList items READ get_items_qvariant_list NOTIFY items_changed)
	Q_PROPERTY(int prowess READ get_prowess NOTIFY prowess_changed)
	Q_PROPERTY(int wealth READ get_wealth WRITE set_wealth NOTIFY wealth_changed)

public:
	static constexpr const char *class_identifier = "character";
	static constexpr const char *database_folder = "characters";
	static constexpr bool history_only = true;

	static std::set<std::string> get_database_dependencies();

	static void remove(character *character);

	static const std::vector<character *> &get_all_living()
	{
		return character::living_characters;
	}

	static const std::vector<character *> &get_all_active()
	{
		return character::get_all_living();
	}

	static void purge_null_characters();

	static character *generate(culture *culture, religion *religion, phenotype *phenotype = nullptr);

private:
	static inline std::vector<character *> living_characters;

public:
	character(const std::string &identifier);
	virtual ~character() override;

	virtual void process_gsml_dated_property(const gsml_property &property, const QDateTime &date) override;
	virtual void initialize_history() override;

	virtual void check_history() const override
	{
		if (this->get_name().empty()) {
			throw std::runtime_error("Character \"" + this->get_identifier() + "\" has no name.");
		}

		if (this->get_culture() == nullptr) {
			throw std::runtime_error("Character \"" + this->get_identifier() + "\" has no culture.");
		}

		if (this->get_religion() == nullptr) {
			throw std::runtime_error("Character \"" + this->get_identifier() + "\" has no religion.");
		}

		if (this->get_phenotype() == nullptr) {
			throw std::runtime_error("Character \"" + this->get_identifier() + "\" has no phenotype.");
		}
	}

	void do_month();
	void do_year();

	virtual std::string get_name() const override
	{
		return this->name;
	}

	void set_name(const std::string &name);

	void set_name_qstring(const QString &name)
	{
		this->set_name(name.toStdString());
	}

	std::string get_full_name() const;

	QString get_full_name_qstring() const
	{
		return QString::fromStdString(this->get_full_name());
	}

	std::string get_titled_name() const;

	QString get_titled_name_qstring() const
	{
		return QString::fromStdString(this->get_titled_name());
	}

	bool is_alive() const
	{
		return this->alive;
	}

	void set_alive(const bool alive);

	bool is_female() const
	{
		return this->female;
	}

	metternich::dynasty *get_dynasty() const
	{
		return this->dynasty;
	}

	void set_dynasty(dynasty *dynasty)
	{
		if (dynasty == this->get_dynasty()) {
			return;
		}

		this->dynasty = dynasty;

		emit dynasty_changed();
	}

	metternich::culture *get_culture() const
	{
		return this->culture;
	}

	void set_culture(culture *culture);

	metternich::religion *get_religion() const
	{
		return this->religion;
	}

	metternich::phenotype *get_phenotype() const
	{
		return this->phenotype;
	}

	landed_title *get_primary_title() const
	{
		return this->primary_title;
	}

	void set_primary_title(landed_title *title);
	void choose_primary_title();

	const std::vector<landed_title *> &get_landed_titles() const
	{
		return this->landed_titles;
	}

	bool has_landed_title(const landed_title *title) const
	{
		return std::find(this->landed_titles.begin(), this->landed_titles.end(), title) != this->landed_titles.end();
	}

	bool is_landed() const
	{
		return this->get_primary_title() != nullptr;
	}

	void add_landed_title(landed_title *title);
	void remove_landed_title(landed_title *title);

	void add_holding(holding *holding)
	{
		this->holdings.push_back(holding);
	}

	void remove_holding(holding *holding);

	character *get_father() const
	{
		return this->father;
	}

	void set_father(character *father);

	character *get_mother() const
	{
		return this->mother;
	}

	void set_mother(character *mother);

	character *get_spouse() const
	{
		return this->spouse;
	}

	void set_spouse(character *spouse)
	{
		if (this->get_spouse() == spouse) {
			return;
		}

		if (this->get_spouse() != nullptr) {
			this->get_spouse()->spouse = nullptr;
		}

		this->spouse = spouse;
		spouse->spouse = this;
	}

	const QDateTime &get_birth_date()
	{
		return this->birth_date;
	}

	const QDateTime &get_death_date()
	{
		return this->death_date;
	}

	character *get_liege() const
	{
		return this->liege;
	}

	void set_liege(character *liege);

	character *get_top_liege() const
	{
		if (this->get_liege() != nullptr) {
			return this->get_liege()->get_top_liege();
		}

		return const_cast<character *>(this);
	}

	bool is_any_liege_of(const character *character) const
	{
		if (character->get_liege() == nullptr) {
			return false;
		} else if (this == character->get_liege()) {
			return true;
		}

		return this->is_any_liege_of(character->get_liege());
	}

	landed_title *get_realm() const
	{
		character *top_liege = this->get_top_liege();
		return top_liege->get_primary_title();
	}

	province *get_capital_province() const;

	province *get_location() const
	{
		return this->get_capital_province();
	}

	const std::vector<trait *> &get_traits() const
	{
		return this->traits;
	}

	QVariantList get_traits_qvariant_list() const;

	Q_INVOKABLE void add_trait(trait *trait);
	Q_INVOKABLE void remove_trait(trait *trait);

	bool has_trait(const trait *trait) const
	{
		return std::find(this->traits.begin(), this->traits.end(), trait) != this->traits.end();
	}

	bool has_personality_trait() const;
	void generate_personality_trait();

	const std::vector<item *> &get_items() const
	{
		return this->items;
	}

	QVariantList get_items_qvariant_list() const;

	Q_INVOKABLE void add_item(item *item);
	Q_INVOKABLE void remove_item(item *item);

	bool has_item(const item *item) const
	{
		return std::find(this->items.begin(), this->items.end(), item) != this->items.end();
	}

	government_type *get_government_type() const
	{
		return this->government_type;
	}

	void set_government_type(government_type *government_type);
	void calculate_government_type();

	bool has_law(law *law) const;

	int get_prowess() const
	{
		return this->prowess;
	}

	void set_prowess(const int prowess)
	{
		if (this->prowess == prowess) {
			return;
		}

		this->prowess = prowess;
		emit prowess_changed();
	}

	void change_prowess(const int change)
	{
		this->set_prowess(this->get_prowess() + change);
	}

	int get_wealth() const
	{
		return this->wealth;
	}

	void set_wealth(const int wealth)
	{
		if (this->wealth == wealth) {
			return;
		}

		this->wealth = wealth;
		emit wealth_changed();
	}

	void change_wealth(const int change)
	{
		this->set_wealth(this->get_wealth() + change);
	}

	int get_stored_commodity(const commodity *commodity) const
	{
		auto find_iterator = this->stored_commodities.find(commodity);
		if (find_iterator == this->stored_commodities.end()) {
			return 0;
		}

		return find_iterator->second;
	}

	bool is_ai() const;

	bool can_build_in_holding(const holding *holding);
	Q_INVOKABLE bool can_build_in_holding(const QVariant &holding_variant);

	bool has_flag(const scoped_flag<character> *flag) const
	{
		return this->flags.contains(flag);
	}

	void add_flag(const scoped_flag<character> *flag)
	{
		if (this->has_flag(flag)) {
			return;
		}

		this->flags.insert(flag);
		emit flags_changed();
	}

	void remove_flag(const scoped_flag<character> *flag)
	{
		if (!this->has_flag(flag)) {
			return;
		}

		this->flags.erase(flag);
		emit flags_changed();
	}

	Q_INVOKABLE QVariantList get_targeted_decisions(const QVariant &target_variant);

private:
	template <typename decision_type, typename T>
	QVariantList get_targeted_decisions(const T *scope)
	{
		QVariantList decision_list;
		QVariantList disabled_decision_list;

		for (decision_type *decision : decision_type::get_all()) {
			if (!decision->check_filter(scope, this)) {
				continue;
			}

			if (!decision->check_preconditions(scope, this)) {
				continue;
			}

			if (!decision->check_conditions(scope, this)) {
				disabled_decision_list.append(QVariant::fromValue(decision));
				continue;
			}

			decision_list.append(QVariant::fromValue(decision));
		}

		for (const QVariant &variant : disabled_decision_list) {
			decision_list.append(variant);
		}

		return decision_list;
	}

signals:
	void name_changed();
	void full_name_changed();
	void titled_name_changed();
	void alive_changed();
	void dynasty_changed();
	void culture_changed();
	void religion_changed();
	void primary_title_changed();
	void liege_changed();
	void traits_changed();
	void items_changed();
	void government_type_changed();
	void prowess_changed();
	void wealth_changed();
	void laws_changed();
	void flags_changed();
	void capital_province_changed();
	void location_changed();
	void ai_changed();

private:
	std::string name;
	bool alive = true;
	bool female = false;
	metternich::dynasty *dynasty = nullptr;
	metternich::culture *culture = nullptr;
	metternich::religion *religion = nullptr;
	metternich::phenotype *phenotype = nullptr;
	landed_title *primary_title = nullptr;
	std::vector<landed_title *> landed_titles;
	std::vector<holding *> holdings;
	character *father = nullptr;
	character *mother = nullptr;
	std::vector<character *> children;
	character *spouse = nullptr;
	QDateTime birth_date;
	QDateTime death_date;
	character *liege = nullptr;
	std::vector<character *> vassals;
	std::vector<trait *> traits;
	std::vector<item *> items;
	government_type *government_type = nullptr;
	int prowess = 0; //the character's skill in personal combat
	int wealth = 0;
	std::map<const commodity *, int> stored_commodities; //the amount of each commodity stored by the character
	std::unique_ptr<condition_check<character>> government_condition_check;
	std::set<const scoped_flag<character> *> flags;
};

}
