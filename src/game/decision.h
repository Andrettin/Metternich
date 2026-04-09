#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

Q_MOC_INCLUDE("ui/icon.h")

namespace metternich {

class domain;
class icon;
enum class decision_type;

template <typename scope_type>
class and_condition;

template <typename scope_type>
class effect_list;

class decision final : public named_data_entry, public data_type<decision>
{
	Q_OBJECT

	Q_PROPERTY(metternich::decision_type type MEMBER type READ get_type NOTIFY changed)
	Q_PROPERTY(const metternich::icon* icon MEMBER icon READ get_icon NOTIFY changed)
	Q_PROPERTY(QString description READ get_description_qstring NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "decision";
	static constexpr const char property_class_identifier[] = "metternich::decision*";
	static constexpr const char database_folder[] = "decisions";

	explicit decision(const std::string &identifier);
	~decision();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	decision_type get_type() const
	{
		return this->type;
	}

	const metternich::icon *get_icon() const
	{
		return this->icon;
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

	const and_condition<domain> *get_conditions() const
	{
		return this->conditions.get();
	}

	Q_INVOKABLE QString get_conditions_string(const metternich::domain *domain) const;

	const effect_list<const domain> *get_effects() const
	{
		return this->effects.get();
	}

	Q_INVOKABLE QString get_effects_string(const metternich::domain *domain) const;

	Q_INVOKABLE bool can_be_enacted_by(const metternich::domain *domain) const;
	[[nodiscard]] QCoro::Task<void> enact_for_coro(const metternich::domain *domain) const;

	Q_INVOKABLE [[nodiscard]] QCoro::QmlTask enact_for(const metternich::domain *domain) const
	{
		return this->enact_for_coro(domain);
	}

signals:
	void changed();

private:
	decision_type type{};
	const metternich::icon *icon = nullptr;
	std::string description;
	std::unique_ptr<and_condition<domain>> conditions;
	std::unique_ptr<effect_list<const domain>> effects;
};

}
