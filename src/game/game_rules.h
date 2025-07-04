#pragma once

#include "game/game_rule_container.h"
#include "game/game_rules_base.h"
#include "util/qunique_ptr.h"

namespace archimedes {
	class game_rule;
	class gsml_data;
	class gsml_property;
}

namespace metternich {

class game_rules final : public game_rules_base
{
	Q_OBJECT

	Q_PROPERTY(QVariantList rules READ get_rules_qvariant_list CONSTANT)
	Q_PROPERTY(QVariantList values READ get_values_qvariant_list NOTIFY values_changed)

public:
	game_rules();

	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);

	gsml_data to_gsml_data() const;

	qunique_ptr<game_rules> duplicate() const
	{
		auto duplicate = make_qunique<game_rules>();
		duplicate->values = this->get_values();
		return duplicate;
	}

	QVariantList get_rules_qvariant_list() const;

	const game_rule_map<bool> &get_values() const
	{
		return this->values;
	}

	QVariantList get_values_qvariant_list() const;
	Q_INVOKABLE virtual bool get_value(const archimedes::game_rule *rule) const override;
	Q_INVOKABLE bool get_value(const std::string &rule_identifier) const;
	Q_INVOKABLE void set_value(const archimedes::game_rule *rule, const bool value);

	Q_INVOKABLE bool is_rule_available(const archimedes::game_rule *rule) const;

signals:
	void values_changed();

private:
	game_rule_map<bool> values;
};

}
