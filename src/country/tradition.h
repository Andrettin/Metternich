#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

Q_MOC_INCLUDE("country/tradition_group.h")
Q_MOC_INCLUDE("technology/technology.h")
Q_MOC_INCLUDE("ui/icon.h")
Q_MOC_INCLUDE("ui/portrait.h")

namespace metternich {

class icon;
class portrait;
class technology;
class tradition_group;
enum class tradition_category;

template <typename scope_type>
class condition;

template <typename scope_type>
class modifier;

class tradition final : public named_data_entry, public data_type<tradition>
{
	Q_OBJECT

	Q_PROPERTY(metternich::tradition_category category MEMBER category READ get_category NOTIFY changed)
	Q_PROPERTY(QString category_name READ get_category_name_qstring NOTIFY changed)
	Q_PROPERTY(metternich::tradition_group* group MEMBER group NOTIFY changed)
	Q_PROPERTY(const metternich::portrait* portrait MEMBER portrait READ get_portrait NOTIFY changed)
	Q_PROPERTY(const metternich::icon* icon MEMBER icon READ get_icon NOTIFY changed)
	Q_PROPERTY(metternich::technology* required_technology MEMBER required_technology NOTIFY changed)
	Q_PROPERTY(const QObject* tree_parent READ get_tree_parent CONSTANT)
	Q_PROPERTY(QVariantList secondary_tree_parents READ get_secondary_tree_parents CONSTANT)

public:
	static constexpr const char class_identifier[] = "tradition";
	static constexpr const char property_class_identifier[] = "metternich::tradition*";
	static constexpr const char database_folder[] = "traditions";

	explicit tradition(const std::string &identifier);
	~tradition();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;
	virtual void check() const override;

	tradition_category get_category() const
	{
		return this->category;
	}

	QString get_category_name_qstring() const;

	const tradition_group *get_group() const
	{
		return this->group;
	}

	const portrait *get_portrait() const
	{
		return this->portrait;
	}

	const icon *get_icon() const
	{
		return this->icon;
	}

	const technology *get_required_technology() const
	{
		return this->required_technology;
	}

	const std::vector<tradition *> &get_prerequisites() const
	{
		return this->prerequisites;
	}

	int get_total_prerequisite_depth() const
	{
		return this->total_prerequisite_depth;
	}

	void calculate_total_prerequisite_depth();

	const std::vector<const tradition *> &get_incompatible_traditions() const
	{
		return this->incompatible_traditions;
	}

	const condition<country> *get_preconditions() const
	{
		return this->preconditions.get();
	}

	const condition<country> *get_conditions() const
	{
		return this->conditions.get();
	}

	Q_INVOKABLE QString get_requirements_string(const metternich::country *country) const;

	const modifier<const country> *get_modifier() const
	{
		return this->modifier.get();
	}

	Q_INVOKABLE QString get_modifier_string(const metternich::country *country) const;

	bool is_available_for_country(const country *country) const;

	virtual named_data_entry *get_tree_parent() const override
	{
		if (!this->get_prerequisites().empty()) {
			return this->get_prerequisites().front();
		}

		return nullptr;
	}

	QVariantList get_secondary_tree_parents() const
	{
		QVariantList secondary_tree_parents;

		for (size_t i = 1; i < this->get_prerequisites().size(); ++i) {
			secondary_tree_parents.push_back(QVariant::fromValue(this->get_prerequisites()[i]));
		}

		return secondary_tree_parents;
	}

	virtual int get_tree_y() const override
	{
		return this->get_total_prerequisite_depth();
	}

	virtual std::vector<const named_data_entry *> get_top_tree_elements() const override
	{
		std::vector<const named_data_entry *> top_tree_elements;

		for (const tradition *tradition : tradition::get_all()) {
			if (!tradition->get_prerequisites().empty()) {
				continue;
			}

			top_tree_elements.push_back(tradition);
		}

		return top_tree_elements;
	}

	virtual bool is_hidden_in_tree() const override;

signals:
	void changed();

private:
	tradition_category category{};
	tradition_group *group = nullptr;
	const portrait *portrait = nullptr;
	const icon *icon = nullptr;
	technology *required_technology = nullptr;
	std::vector<tradition *> prerequisites;
	int total_prerequisite_depth = 0;
	std::vector<const tradition *> incompatible_traditions;
	std::unique_ptr<const condition<country>> preconditions;
	std::unique_ptr<const condition<country>> conditions;
	std::unique_ptr<const modifier<const country>> modifier;
};

}
