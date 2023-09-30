#pragma once

#include "util/image_provider_base.h"

namespace metternich {

class tile_image_provider final : public image_provider_base
{
public:
	static tile_image_provider *get()
	{
		return tile_image_provider::instance;
	}

private:
	static inline tile_image_provider *instance = nullptr;

public:
	tile_image_provider();

	virtual QCoro::Task<void> load_image(const std::string id) override;
};

}
