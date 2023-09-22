#include "metternich.h"

#include "script/text_processor.h"

#include "country/culture.h"
#include "infrastructure/settlement_type.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "population/population_type.h"
#include "population/population_unit.h"
#include "util/assert_util.h"
#include "util/queue_util.h"
#include "util/string_util.h"

namespace metternich {

std::string text_processor::process_tokens(std::queue<std::string> &&tokens, const bool process_in_game_data, bool &processed) const
{
	Q_UNUSED(process_in_game_data);

	processed = true;

	if (tokens.empty()) {
		return std::string();
	}

	const std::string token = queue::take(tokens);
	std::queue<std::string> subtokens = text_processor::get_subtokens(token);

	const std::string front_subtoken = queue::take(subtokens);

	std::string str;

	if (front_subtoken == "root") {
		str = this->process_scope_variant_tokens(this->context.root_scope, tokens);
	} else if (front_subtoken == "source") {
		str = this->process_scope_variant_tokens(this->context.source_scope, tokens);
	} else if (front_subtoken == "saved_population_unit_scope") {
		const std::string scope_name = queue::take(subtokens);
		str = this->process_population_unit_tokens(this->context.get_saved_scope<const population_unit>(scope_name), tokens);
	} else if (front_subtoken == "saved_site_scope") {
		const std::string scope_name = queue::take(subtokens);
		str = this->process_site_tokens(this->context.get_saved_scope<const site>(scope_name), tokens);
	} else {
		throw std::runtime_error("Failed to process token \"" + token + "\".");
	}

	if (!tokens.empty()) {
		return this->process_string_tokens(std::move(str), std::move(tokens));
	}

	return str;
}

std::string text_processor::process_scope_variant_tokens(const read_only_context::scope_variant_type &scope_variant, std::queue<std::string> &tokens) const
{
	if (std::holds_alternative<const province *>(scope_variant)) {
		return this->process_province_tokens(std::get<const province *>(scope_variant), tokens);
	} else if (std::holds_alternative<const site *>(scope_variant)) {
		return this->process_site_tokens(std::get<const site *>(scope_variant), tokens);
	} else {
		assert_throw(false);
	}

	return std::string();
}

std::string text_processor::process_culture_tokens(const culture *culture, std::queue<std::string> &tokens) const
{
	if (culture == nullptr) {
		throw std::runtime_error("No culture provided when processing culture tokens.");
	}

	if (tokens.empty()) {
		throw std::runtime_error("No tokens provided when processing culture tokens.");
	}

	const std::string token = queue::take(tokens);
	std::queue<std::string> subtokens = text_processor::get_subtokens(token);

	const std::string front_subtoken = queue::take(subtokens);

	if (front_subtoken == "name") {
		return culture->get_name();
	} else {
		return this->process_named_data_entry_token(culture, front_subtoken);
	}
}

std::string text_processor::process_population_type_tokens(const population_type *population_type, std::queue<std::string> &tokens) const
{
	if (population_type == nullptr) {
		throw std::runtime_error("No population type provided when processing population type tokens.");
	}

	if (tokens.empty()) {
		throw std::runtime_error("No tokens provided when processing population type tokens.");
	}

	const std::string token = queue::take(tokens);
	std::queue<std::string> subtokens = text_processor::get_subtokens(token);

	const std::string front_subtoken = queue::take(subtokens);

	if (front_subtoken == "name") {
		return population_type->get_name();
	} else {
		return this->process_named_data_entry_token(population_type, front_subtoken);
	}
}

std::string text_processor::process_population_unit_tokens(const population_unit *population_unit, std::queue<std::string> &tokens) const
{
	if (population_unit == nullptr) {
		throw std::runtime_error("No population unit provided when processing population unit tokens.");
	}

	if (tokens.empty()) {
		throw std::runtime_error("No tokens provided when processing population unit tokens.");
	}

	const std::string token = queue::take(tokens);
	std::queue<std::string> subtokens = text_processor::get_subtokens(token);

	const std::string front_subtoken = queue::take(subtokens);

	if (front_subtoken == "culture") {
		return this->process_culture_tokens(population_unit->get_culture(), tokens);
	} else if (front_subtoken == "type") {
		return this->process_population_type_tokens(population_unit->get_type(), tokens);
	}

	throw std::runtime_error(std::format("Failed to process population unit token \"{}\".", token));
}

std::string text_processor::process_province_tokens(const province *province, std::queue<std::string> &tokens) const
{
	if (province == nullptr) {
		throw std::runtime_error("No province provided when processing province tokens.");
	}

	if (tokens.empty()) {
		throw std::runtime_error("No tokens provided when processing province tokens.");
	}

	const std::string token = queue::take(tokens);
	std::queue<std::string> subtokens = text_processor::get_subtokens(token);

	const std::string front_subtoken = queue::take(subtokens);

	if (front_subtoken == "name") {
		return province->get_game_data()->get_current_cultural_name();
	} else {
		return this->process_named_data_entry_token(province, front_subtoken);
	}
}

std::string text_processor::process_settlement_type_tokens(const settlement_type *settlement_type, std::queue<std::string> &tokens) const
{
	if (settlement_type == nullptr) {
		throw std::runtime_error("No settlement type provided when processing settlement type tokens.");
	}

	if (tokens.empty()) {
		throw std::runtime_error("No tokens provided when processing settlement type tokens.");
	}

	const std::string token = queue::take(tokens);
	std::queue<std::string> subtokens = text_processor::get_subtokens(token);

	const std::string front_subtoken = queue::take(subtokens);

	if (front_subtoken == "name") {
		return settlement_type->get_name();
	} else {
		return this->process_named_data_entry_token(settlement_type, front_subtoken);
	}
}

std::string text_processor::process_site_tokens(const site *site, std::queue<std::string> &tokens) const
{
	if (site == nullptr) {
		throw std::runtime_error("No site provided when processing site tokens.");
	}

	if (tokens.empty()) {
		throw std::runtime_error("No tokens provided when processing site tokens.");
	}

	const std::string token = queue::take(tokens);
	std::queue<std::string> subtokens = text_processor::get_subtokens(token);

	const std::string front_subtoken = queue::take(subtokens);

	if (front_subtoken == "name") {
		return site->get_game_data()->get_current_cultural_name();
	} else if (front_subtoken == "province") {
		return this->process_province_tokens(site->get_game_data()->get_province(), tokens);
	} else if (front_subtoken == "settlement_type") {
		return this->process_settlement_type_tokens(site->get_game_data()->get_settlement_type(), tokens);
	} else {
		return this->process_named_data_entry_token(site, front_subtoken);
	}
}

}
