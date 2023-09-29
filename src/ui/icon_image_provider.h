#pragma once

#include "util/image_provider_base.h"

namespace metternich {

class icon_image_provider final : public image_provider_base
{
public:
	static icon_image_provider *get()
	{
		return icon_image_provider::instance;
	}

private:
	static inline icon_image_provider *instance = nullptr;

public:
	icon_image_provider();

	virtual QCoro::Task<void> load_image(const std::string id) override;
};

}
