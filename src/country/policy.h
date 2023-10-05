#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "economy/commodity_container.h"

namespace metternich {

class country;

template <typename scope_type>
class modifier;

class policy final : public named_data_entry, public data_type<policy>
{
	Q_OBJECT

	Q_PROPERTY(QString left_name READ get_left_name_qstring NOTIFY changed)
	Q_PROPERTY(QString right_name READ get_right_name_qstring NOTIFY changed)
	Q_PROPERTY(QVariantList change_commodity_costs READ get_change_commodity_costs_qvariant_list NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "policy";
	static constexpr const char property_class_identifier[] = "metternich::policy*";
	static constexpr const char database_folder[] = "policies";

	static constexpr int min_value = -5;
	static constexpr int max_value = 5;

	explicit policy(const std::string &identifier);
	~policy();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;
	virtual void check() const override;

	const std::string &get_left_name() const
	{
		return this->left_name;
	}

	Q_INVOKABLE void set_left_name(const std::string &name)
	{
		this->left_name = name;
	}

	QString get_left_name_qstring() const
	{
		return QString::fromStdString(this->get_left_name());
	}

	const std::string &get_right_name() const
	{
		return this->right_name;
	}

	Q_INVOKABLE void set_right_name(const std::string &name)
	{
		this->right_name = name;
	}

	QString get_right_name_qstring() const
	{
		return QString::fromStdString(this->get_right_name());
	}

	const commodity_map<int> &get_change_commodity_costs() const
	{
		return this->change_commodity_costs;
	}

	QVariantList get_change_commodity_costs_qvariant_list() const;

	const metternich::modifier<const country> *get_modifier() const
	{
		return this->modifier.get();
	}

	const metternich::modifier<const country> *get_left_modifier() const
	{
		return this->left_modifier.get();
	}

	const metternich::modifier<const country> *get_right_modifier() const
	{
		return this->right_modifier.get();
	}

	void apply_modifier(const country *country, const int value, const int multiplier) const;
	Q_INVOKABLE QString get_modifier_string(const metternich::country *country, const int value) const;

signals:
	void changed();

private:
	std::string left_name;
	std::string right_name;
	commodity_map<int> change_commodity_costs;
	std::unique_ptr<const metternich::modifier<const country>> modifier;
	std::unique_ptr<const metternich::modifier<const country>> left_modifier;
	std::unique_ptr<const metternich::modifier<const country>> right_modifier;
};

}
