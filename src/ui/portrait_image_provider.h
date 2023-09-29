#pragma once

#include "util/image_provider_base.h"

namespace metternich {

class portrait_image_provider final : public image_provider_base
{
public:
	static portrait_image_provider *get()
	{
		return portrait_image_provider::instance;
	}

private:
	static inline portrait_image_provider *instance = nullptr;

public:
	portrait_image_provider();

	virtual QCoro::Task<void> load_image(const std::string id) override;
};

}
