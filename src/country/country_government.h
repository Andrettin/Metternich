#pragma once

#include "country/law_group_container.h"
#include "database/data_entry_container.h"
#include "economy/commodity_container.h"

Q_MOC_INCLUDE("country/government_type.h")
Q_MOC_INCLUDE("country/law.h")
Q_MOC_INCLUDE("country/law_group.h")

namespace metternich {

class character;
class country;
class country_game_data;
class government_type;
class law;
class office;

class country_government final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(const metternich::government_type *government_type READ get_government_type NOTIFY government_type_changed)
	Q_PROPERTY(QVariantList laws READ get_laws_qvariant_list NOTIFY laws_changed)
	Q_PROPERTY(const metternich::character* ruler READ get_ruler NOTIFY ruler_changed)
	Q_PROPERTY(QVariantList office_holders READ get_office_holders_qvariant_list NOTIFY office_holders_changed)
	Q_PROPERTY(QVariantList appointed_office_holders READ get_appointed_office_holders_qvariant_list NOTIFY appointed_office_holders_changed)
	Q_PROPERTY(QVariantList available_offices READ get_available_offices_qvariant_list NOTIFY available_offices_changed)
	Q_PROPERTY(int advisor_cost READ get_advisor_cost NOTIFY office_holders_changed)
	Q_PROPERTY(const metternich::portrait* interior_minister_portrait READ get_interior_minister_portrait NOTIFY office_holders_changed)
	Q_PROPERTY(const metternich::portrait* war_minister_portrait READ get_war_minister_portrait NOTIFY office_holders_changed)

public:
	static constexpr int base_advisor_cost = 80;

	explicit country_government(const metternich::country *country, const country_game_data *game_data);
	~country_government();

	country_game_data *get_game_data() const;

	const std::string &get_office_title_name(const office *office) const;

	Q_INVOKABLE QString get_office_title_name_qstring(const metternich::office *office) const
	{
		return QString::fromStdString(this->get_office_title_name(office));
	}

	const metternich::government_type *get_government_type() const
	{
		return this->government_type;
	}

	void set_government_type(const metternich::government_type *government_type);
	bool can_have_government_type(const metternich::government_type *government_type) const;
	void check_government_type();

	bool is_tribal() const;
	bool is_clade() const;

	const law_group_map<const law *> &get_laws() const
	{
		return this->laws;
	}

	QVariantList get_laws_qvariant_list() const;

	Q_INVOKABLE const metternich::law *get_law(const metternich::law_group *law_group) const
	{
		const auto find_iterator = this->get_laws().find(law_group);

		if (find_iterator != this->get_laws().end()) {
			return find_iterator->second;
		}

		return nullptr;
	}

	void set_law(const law_group *law_group, const law *law);
	bool has_law(const law *law) const;
	Q_INVOKABLE bool can_have_law(const metternich::law *law) const;
	Q_INVOKABLE bool can_enact_law(const metternich::law *law) const;
	Q_INVOKABLE void enact_law(const metternich::law *law);
	Q_INVOKABLE int get_total_law_cost_modifier() const;
	void check_laws();

	const character *get_ruler() const;

	const data_entry_map<office, const character *> &get_office_holders() const
	{
		return this->office_holders;
	}

	QVariantList get_office_holders_qvariant_list() const;

	Q_INVOKABLE const metternich::character *get_office_holder(const metternich::office *office) const
	{
		const auto find_iterator = this->office_holders.find(office);

		if (find_iterator != this->office_holders.end()) {
			return find_iterator->second;
		}

		return nullptr;
	}

	void set_office_holder(const office *office, const character *character);

	const data_entry_map<office, const character *> &get_appointed_office_holders() const
	{
		return this->appointed_office_holders;
	}

	QVariantList get_appointed_office_holders_qvariant_list() const;

	Q_INVOKABLE const metternich::character *get_appointed_office_holder(const metternich::office *office) const
	{
		const auto find_iterator = this->appointed_office_holders.find(office);

		if (find_iterator != this->appointed_office_holders.end()) {
			return find_iterator->second;
		}

		return nullptr;
	}

	Q_INVOKABLE void set_appointed_office_holder(const metternich::office *office, const metternich::character *character);

	void check_office_holder(const office *office, const character *previous_holder);
	void check_office_holders();
	std::vector<const character *> get_appointable_office_holders(const office *office) const;
	Q_INVOKABLE QVariantList get_appointable_office_holders_qvariant_list(const metternich::office *office) const;
	const character *get_best_office_holder(const office *office, const character *previous_holder) const;
	bool can_have_office_holder(const office *office, const character *character) const;
	bool can_gain_office_holder(const office *office, const character *character) const;
	Q_INVOKABLE bool can_appoint_office_holder(const metternich::office *office, const metternich::character *character) const;
	void on_office_holder_died(const office *office, const character *office_holder);

	std::vector<const office *> get_available_offices() const;
	std::vector<const office *> get_appointable_available_offices() const;
	QVariantList get_available_offices_qvariant_list() const;

	int get_advisor_cost() const
	{
		int cost = 0;

		const int advisor_count = static_cast<int>(this->get_office_holders().size() + this->get_appointed_office_holders().size()) - 1;

		if (advisor_count <= 0) {
			cost = country_government::base_advisor_cost / 2;
		} else {
			cost = country_government::base_advisor_cost * (advisor_count + 1);
		}

		return std::max(0, cost);
	}

	commodity_map<int> get_advisor_commodity_costs(const office *office) const;
	Q_INVOKABLE QVariantList get_advisor_commodity_costs_qvariant_list(const metternich::office *office) const;

	bool can_have_appointable_offices() const;

	const metternich::portrait *get_interior_minister_portrait() const;
	const metternich::portrait *get_war_minister_portrait() const;

	int get_law_cost_modifier() const
	{
		return this->law_cost_modifier;
	}

	void change_law_cost_modifier(const int change)
	{
		this->law_cost_modifier += change;
	}

signals:
	void office_title_names_changed();
	void government_type_changed();
	void laws_changed();
	void ruler_changed();
	void office_holders_changed();
	void appointed_office_holders_changed();
	void available_offices_changed();

private:
	const metternich::country *country = nullptr;
	const metternich::government_type *government_type = nullptr;
	law_group_map<const law *> laws;
	data_entry_map<office, const character *> office_holders;
	data_entry_map<office, const character *> appointed_office_holders;
	int law_cost_modifier = 0;
};

}
