#include "metternich.h"

#include "script/context.h"

#include "character/character.h"
#include "database/gsml_data.h"
#include "domain/domain.h"
#include "game/game.h"
#include "map/province.h"
#include "map/site.h"
#include "population/population_unit.h"
#include "script/special_target_type.h"
#include "util/assert_util.h"
#include "util/string_conversion_util.h"

namespace metternich {

template <bool read_only>
void context_base<read_only>::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "root_character") {
		this->root_scope = game::get()->get_character(value);
	} else if (key == "root_domain") {
		this->root_scope = domain::get(value);
	} else if (key == "root_province") {
		this->root_scope = province::get(value);
	} else if (key == "root_site") {
		this->root_scope = site::get(value);
	} else if (key == "source_character") {
		this->source_scope = game::get()->get_character(value);
	} else if (key == "source_domain") {
		this->source_scope = domain::get(value);
	} else if (key == "source_province") {
		this->source_scope = province::get(value);
	} else if (key == "source_site") {
		this->source_scope = site::get(value);
	} else if (key == "ruin_site") {
		this->ruin_site = site::get(value);
	} else if (key == "in_combat") {
		this->in_combat = string::to_bool(value);
	} else {
		throw std::runtime_error(std::format("Invalid context property: \"{}\".", key));
	}
}

template <bool read_only>
void context_base<read_only>::process_gsml_scope(const gsml_data &scope)
{
	throw std::runtime_error("Invalid context scope: \"" + scope.get_tag() + "\".");
}

template <bool read_only>
gsml_data context_base<read_only>::to_gsml_data(const std::string &tag) const
{
	gsml_data data(tag);

	if (std::holds_alternative<const character *>(this->root_scope)) {
		data.add_property("root_character", std::get<const character *>(this->root_scope)->get_identifier());
	} else if (std::holds_alternative<const domain *>(this->root_scope)) {
		data.add_property("root_domain", std::get<const domain *>(this->root_scope)->get_identifier());
	} else if (std::holds_alternative<const province *>(this->root_scope)) {
		data.add_property("root_province", std::get<const province *>(this->root_scope)->get_identifier());
	} else if (std::holds_alternative<const site *>(this->root_scope)) {
		data.add_property("root_site", std::get<const site *>(this->root_scope)->get_identifier());
	} else {
		assert_throw(false);
	}

	if (std::holds_alternative<const character *>(this->source_scope)) {
		data.add_property("source_character", std::get<const character *>(this->source_scope)->get_identifier());
	} else if (std::holds_alternative<const domain *>(this->source_scope)) {
		data.add_property("source_domain", std::get<const domain *>(this->source_scope)->get_identifier());
	} else if (std::holds_alternative<const province *>(this->source_scope)) {
		data.add_property("source_province", std::get<const province *>(this->source_scope)->get_identifier());
	} else if (std::holds_alternative<const site *>(this->source_scope)) {
		data.add_property("source_site", std::get<const site *>(this->source_scope)->get_identifier());
	} else {
		assert_throw(false);
	}

	if (this->ruin_site != nullptr) {
		data.add_property("ruin_site", this->ruin_site->get_identifier());
	}

	if (this->in_combat) {
		data.add_property("in_combat", string::from_bool(this->in_combat));
	}

	return data;
}

template <bool read_only>
const typename context_base<read_only>::scope_variant_type &context_base<read_only>::get_special_target_scope_variant(const special_target_type special_target_type) const
{
	switch (special_target_type) {
		case special_target_type::root:
			return this->root_scope;
		case special_target_type::source:
			return this->source_scope;
		case special_target_type::previous:
			return this->previous_scope;
		default:
			throw std::runtime_error(std::format("Invalid special target type value: {}.", static_cast<int>(special_target_type)));
	}
}

template struct context_base<false>;
template struct context_base<true>;

read_only_context::scope_variant_type read_only_context::scope_from_mutable(const context_base<false>::scope_variant_type &mutable_scope)
{
	scope_variant_type scope;

	std::visit([&scope](auto &&arg) {
		scope = arg;
	}, mutable_scope);

	return scope;
}

read_only_context::read_only_context(const context &ctx)
	: read_only_context(read_only_context::scope_from_mutable(ctx.root_scope))
{
	this->source_scope = read_only_context::scope_from_mutable(ctx.source_scope);
	this->previous_scope = read_only_context::scope_from_mutable(ctx.previous_scope);

	this->saved_character_scopes = ctx.saved_character_scopes;
	this->saved_domain_scopes = ctx.saved_domain_scopes;
	this->saved_province_scopes = ctx.saved_province_scopes;
	this->saved_site_scopes = ctx.saved_site_scopes;
	this->saved_strings = ctx.saved_strings;
	this->saved_buildings = ctx.saved_buildings;

	this->army = ctx.army;
	this->attacking_army = ctx.attacking_army;
	this->defending_army = ctx.defending_army;

	this->party = ctx.party;
	this->enemies = ctx.enemies;
	this->ruin_site = ctx.ruin_site;
	this->in_combat = ctx.in_combat;

	for (const auto &[str, military_unit] : ctx.saved_military_unit_scopes) {
		this->saved_military_unit_scopes[str] = military_unit;
	}
	for (const auto &[str, population_unit] : ctx.saved_population_unit_scopes) {
		this->saved_population_unit_scopes[str] = population_unit;
	}
}

}
