#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

Q_MOC_INCLUDE("country/government_group.h")
Q_MOC_INCLUDE("ui/icon.h")

namespace archimedes {
	enum class gender;
}

namespace metternich {

class government_group;
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

public:
	using government_variant = std::variant<const government_type *, const government_group *>;
	using title_name_map = std::map<country_tier, std::string>;
	using ruler_title_name_map = std::map<country_tier, std::map<gender, std::string>>;
	using landholder_title_name_map = std::map<site_tier, std::map<gender, std::string>>;

	static constexpr const char class_identifier[] = "government_type";
	static constexpr const char property_class_identifier[] = "metternich::government_type*";
	static constexpr const char database_folder[] = "government_types";

	static void process_title_name_scope(std::map<government_variant, title_name_map> &title_names, const gsml_data &scope);
	static void process_title_name_scope(title_name_map &title_names, const gsml_data &scope);
	static void process_ruler_title_name_scope(std::map<government_variant, ruler_title_name_map> &ruler_title_names, const gsml_data &scope);
	static void process_ruler_title_name_scope(ruler_title_name_map &ruler_title_names, const gsml_data &scope);
	static void process_ruler_title_name_scope(std::map<gender, std::string> &ruler_title_names, const gsml_data &scope);
	static void process_landholder_title_name_scope(std::map<government_variant, landholder_title_name_map> &landholder_title_names, const gsml_data &scope);
	static void process_landholder_title_name_scope(landholder_title_name_map &landholder_title_names, const gsml_data &scope);
	static void process_landholder_title_name_scope(std::map<gender, std::string> &landholder_title_names, const gsml_data &scope);

	explicit government_type(const std::string &identifier);
	~government_type();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	const std::string &get_title_name(const country_tier tier) const;
	const std::string &get_ruler_title_name(const country_tier tier, const gender gender) const;
	const std::string &get_landholder_title_name(const site_tier tier, const gender gender) const;

	const government_group *get_group() const
	{
		return this->group;
	}

	const icon *get_icon() const
	{
		return this->icon;
	}

	const and_condition<country> *get_conditions() const
	{
		return this->conditions.get();
	}

signals:
	void changed();

private:
	const government_group *group = nullptr;
	const icon *icon = nullptr;
	std::unique_ptr<const and_condition<country>> conditions;
	title_name_map title_names;
	ruler_title_name_map ruler_title_names;
	landholder_title_name_map landholder_title_names;
};

}
