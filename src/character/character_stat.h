#pragma once

#include "database/named_data_entry.h"
#include "util/centesimal_int.h"

namespace metternich {

class character;

template <typename scope_type>
class modifier;

class character_stat : public named_data_entry
{
	Q_OBJECT

public:
	static const character_stat *get_stat(const std::string &identifier)
	{
		const character_stat *stat = character_stat::try_get_stat(identifier);

		if (stat == nullptr) {
			throw std::runtime_error(std::format("No character stat found for identifier \"{}\".", identifier));
		}

		return stat;
	}

	static const character_stat *try_get_stat(const std::string &identifier)
	{
		const auto find_iterator = character_stat::stats_by_identifier.find(identifier);
		if (find_iterator != character_stat::stats_by_identifier.end()) {
			return find_iterator->second;
		}

		return nullptr;
	}

private:
	static inline std::map<std::string, const character_stat *> stats_by_identifier;

public:
	static const std::unique_ptr<const character_stat> armor_class;

	explicit character_stat(const std::string &identifier);
	~character_stat();

	virtual void process_gsml_scope(const gsml_data &scope) override;

	const modifier<const character> *get_value_modifier(const centesimal_int &value) const
	{
		const auto find_iterator = this->value_modifiers.find(value);
		if (find_iterator != this->value_modifiers.end()) {
			return find_iterator->second.get();
		}

		return nullptr;
	}

	bool is_value_exceptional(const int value) const
	{
		return this->exceptional_values.contains(value);
	}

	bool affects_military_unit_stats() const;

signals:
	void changed();

private:
	bool affect_military_unit_stats = false;
	std::map<centesimal_int, std::unique_ptr<modifier<const character>>> value_modifiers; //the character modifiers applied for each value; these are cumulative
	std::set<int> exceptional_values;
};

}
