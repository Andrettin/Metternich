#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "ui/portrait_container.h"

namespace metternich {

class character;
class country;
class portrait;
enum class advisor_category;

template <typename scope_type>
class condition;

template <typename scope_type>
class modifier;

class advisor_type final : public named_data_entry, public data_type<advisor_type>
{
	Q_OBJECT

	Q_PROPERTY(metternich::advisor_category category MEMBER category NOTIFY changed)
	Q_PROPERTY(metternich::portrait* portrait MEMBER portrait NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "advisor_type";
	static constexpr const char property_class_identifier[] = "metternich::advisor_type*";
	static constexpr const char database_folder[] = "advisor_types";

	explicit advisor_type(const std::string &identifier);

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	advisor_category get_category() const
	{
		return this->category;
	}

	const metternich::portrait *get_portrait() const
	{
		return this->portrait;
	}

	const portrait_map<std::unique_ptr<const condition<character>>> &get_conditional_portraits() const
	{
		return this->conditional_portraits;
	}

	const condition<character> *get_portrait_conditions(const metternich::portrait *portrait) const
	{
		const auto find_iterator = this->conditional_portraits.find(portrait);
		if (find_iterator != this->conditional_portraits.end()) {
			return find_iterator->second.get();
		}

		return nullptr;
	}

	const modifier<const country> *get_modifier() const
	{
		return this->modifier.get();
	}

signals:
	void changed();

private:
	advisor_category category;
	metternich::portrait *portrait = nullptr;
	portrait_map<std::unique_ptr<const condition<character>>> conditional_portraits;
	std::unique_ptr<modifier<const country>> modifier;
};

}
