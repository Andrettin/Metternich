#include "metternich.h"

#include "script/text_processor.h"

#include "map/province.h"
#include "map/province_game_data.h"
#include "map/site.h"
#include "map/site_game_data.h"
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

	std::queue<std::string> subtokens = string::split_to_queue(token, ':');

	if (subtokens.size() > 2) {
		throw std::runtime_error("There can only be at most 2 subtokens.");
	}

	const std::string front_subtoken = queue::take(subtokens);

	std::string str;

	if (front_subtoken == "root") {
		str = this->process_scope_variant_tokens(this->context.root_scope, tokens);
	} else if (front_subtoken == "source") {
		str = this->process_scope_variant_tokens(this->context.source_scope, tokens);
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

std::string text_processor::process_province_tokens(const province *province, std::queue<std::string> &tokens) const
{
	if (province == nullptr) {
		throw std::runtime_error("No province provided when processing province tokens.");
	}

	if (tokens.empty()) {
		throw std::runtime_error("No tokens provided when processing province tokens.");
	}

	const std::string token = queue::take(tokens);

	std::queue<std::string> subtokens = string::split_to_queue(token, ':');

	if (subtokens.size() > 2) {
		throw std::runtime_error("There can only be at most 2 subtokens.");
	}

	const std::string front_subtoken = queue::take(subtokens);

	if (front_subtoken == "name") {
		return province->get_game_data()->get_current_cultural_name();
	} else {
		return this->process_named_data_entry_token(province, front_subtoken);
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

	std::queue<std::string> subtokens = string::split_to_queue(token, ':');

	if (subtokens.size() > 2) {
		throw std::runtime_error("There can only be at most 2 subtokens.");
	}

	const std::string front_subtoken = queue::take(subtokens);

	if (front_subtoken == "name") {
		return site->get_game_data()->get_current_cultural_name();
	} else if (front_subtoken == "location") {
		return this->process_province_tokens(site->get_game_data()->get_province(), tokens);
	} else {
		return this->process_named_data_entry_token(site, front_subtoken);
	}
}

}
