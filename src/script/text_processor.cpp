#include "metternich.h"

#include "script/text_processor.h"

#include "map/province.h"
#include "map/province_game_data.h"
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
		if (std::holds_alternative<const province *>(this->context.root_scope)) {
			str = this->process_province_tokens(std::get<const province *>(this->context.root_scope), tokens);
		} else {
			assert_throw(false);
		}
	} else {
		throw std::runtime_error("Failed to process token \"" + token + "\".");
	}

	if (!tokens.empty()) {
		return this->process_string_tokens(std::move(str), std::move(tokens));
	}

	return str;
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

}
