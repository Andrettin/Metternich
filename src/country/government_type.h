#pragma once

#include "country/law_group_container.h"
#include "database/data_entry_container.h"
#include "database/data_type.h"
#include "database/named_data_entry.h"

Q_MOC_INCLUDE("country/government_group.h")
Q_MOC_INCLUDE("technology/technology.h")
Q_MOC_INCLUDE("ui/icon.h")

namespace archimedes {
	enum class gender;
}

namespace metternich {

class government_group;
class law;
class law_group;
class office;
class technology;
enum class country_tier;
enum class site_tier;

template <typename scope_type>
class and_condition;

template <typename scope_type>
class modifier;

class government_type final : public named_data_entry, public data_type<government_type>
{
	Q_OBJECT

	Q_PROPERTY(const metternich::government_group* group MEMBER group READ get_group NOTIFY changed)
	Q_PROPERTY(const metternich::icon* icon MEMBER icon READ get_icon NOTIFY changed)
	Q_PROPERTY(metternich::technology* required_technology MEMBER required_technology NOTIFY changed)

public:
	using government_variant = std::variant<const government_type *, const government_group *>;
	using title_name_map = std::map<country_tier, std::string>;
	using office_title_inner_name_map = std::map<country_tier, std::map<gender, std::string>>;
	using office_title_name_map = data_entry_map<office, office_title_inner_name_map>;
	using landholder_title_name_map = std::map<site_tier, std::map<gender, std::string>>;

	static constexpr const char class_identifier[] = "government_type";
	static constexpr const char property_class_identifier[] = "metternich::government_type*";
	static constexpr const char database_folder[] = "government_types";

	static void process_title_name_scope(std::map<government_variant, title_name_map> &title_names, const gsml_data &scope);
	static void process_title_name_scope(title_name_map &title_names, const gsml_data &scope);
	static void process_office_title_name_scope(data_entry_map<office, std::map<government_variant, office_title_inner_name_map>> &office_title_names, const gsml_data &scope);
	static void process_office_title_name_scope(office_title_name_map &office_title_names, const gsml_data &scope);
	static void process_office_title_name_scope(std::map<government_variant, office_title_inner_name_map> &office_title_names, const gsml_data &scope);
	static void process_office_title_name_scope(office_title_inner_name_map &office_title_names, const gsml_data &scope);
	static void process_office_title_name_scope(std::map<gender, std::string> &office_title_names, const gsml_data &scope);
	static void process_landholder_title_name_scope(std::map<government_variant, landholder_title_name_map> &landholder_title_names, const gsml_data &scope);
	static void process_landholder_title_name_scope(landholder_title_name_map &landholder_title_names, const gsml_data &scope);
	static void process_landholder_title_name_scope(std::map<gender, std::string> &landholder_title_names, const gsml_data &scope);

	explicit government_type(const std::string &identifier);
	~government_type();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;
	virtual void check() const override;

	const std::string &get_title_name(const country_tier tier) const;
	const std::string &get_office_title_name(const office *office, const country_tier tier, const gender gender) const;
	const std::string &get_landholder_title_name(const site_tier tier, const gender gender) const;

	const government_group *get_group() const
	{
		return this->group;
	}

	const icon *get_icon() const
	{
		return this->icon;
	}

	const std::vector<const law *> &get_forbidden_laws() const
	{
		return this->forbidden_laws;
	}

	const law_group_map<const law *> &get_default_laws() const
	{
		return this->default_laws;
	}

	const law *get_default_law(const law_group *group) const
	{
		const auto find_iterator = this->default_laws.find(group);

		if (find_iterator != this->default_laws.end()) {
			return find_iterator->second;
		}

		return nullptr;
	}

	const technology *get_required_technology() const
	{
		return this->required_technology;
	}

	const and_condition<country> *get_conditions() const
	{
		return this->conditions.get();
	}

	const modifier<const country> *get_modifier() const
	{
		return this->modifier.get();
	}

	Q_INVOKABLE QString get_modifier_string(const metternich::country *country) const;

signals:
	void changed();

private:
	const government_group *group = nullptr;
	const metternich::icon *icon = nullptr;
	std::vector<const law *> forbidden_laws;
	law_group_map<const law *> default_laws;
	technology *required_technology = nullptr;
	std::unique_ptr<const and_condition<country>> conditions;
	std::unique_ptr<const modifier<const country>> modifier;
	title_name_map title_names;
	office_title_name_map office_title_names;
	landholder_title_name_map landholder_title_names;
};

}
