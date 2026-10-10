#pragma once

#include "script/battle_enemy.h"

namespace metternich {

class create_enemy_effect final : public effect<const domain>
{
public:
	explicit create_enemy_effect(const gsml_operator effect_operator)
		: effect<const domain>(effect_operator)
	{
		this->enemy = std::make_unique<battle_enemy>();
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "create_enemy";
		return class_identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		this->enemy->process_gsml_property(property);
	}

	virtual void process_gsml_scope(const gsml_data &scope) override
	{
		this->enemy->process_gsml_scope(scope);
	}

	virtual void check() const override
	{
		assert_throw(this->enemy != nullptr);
		assert_throw(this->enemy->get_military_unit_type() != nullptr);
	}

	virtual void do_assignment_effect(const domain *scope, context &ctx) const override
	{
		Q_UNUSED(scope);

		ctx.enemies.push_back(enemy.get());
	}

	virtual std::string get_assignment_string(const domain *scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const override
	{
		Q_UNUSED(scope);
		Q_UNUSED(ctx);
		Q_UNUSED(indent);
		Q_UNUSED(prefix);

		return {};
	}

	virtual bool is_hidden() const override
	{
		return true;
	}

private:
	std::unique_ptr<battle_enemy> enemy;
};

}
