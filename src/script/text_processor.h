#pragma once

#include "script/context.h"
#include "util/text_processor_base.h"

namespace metternich {

class text_processor final : public text_processor_base
{
public:
	explicit text_processor(const read_only_context &ctx) : context(ctx)
	{
	}

	virtual std::string process_tokens(std::queue<std::string> &&tokens, const bool process_in_game_data, bool &processed) const override;
	std::string process_province_tokens(const province *province, std::queue<std::string> &tokens) const;

private:
	const read_only_context context;
};

}
