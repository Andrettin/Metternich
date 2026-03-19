#pragma once

#include "database/data_entry.h"
#include "database/data_type.h"
#include "util/qunique_ptr.h"

class QSoundEffect;

namespace metternich {

class sound final : public data_entry, public data_type<sound>
{
	Q_OBJECT

	Q_PROPERTY(std::filesystem::path filepath MEMBER filepath WRITE set_filepath)

public:
	static constexpr const char class_identifier[] = "sound";
	static constexpr const char property_class_identifier[] = "metternich::sound*";
	static constexpr const char database_folder[] = "sounds";

	explicit sound(const std::string &identifier);
	~sound();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	const std::filesystem::path &get_filepath() const
	{
		return this->filepath;
	}

	void set_filepath(const std::filesystem::path &filepath);

	[[nodiscard]]
	QCoro::Task<void> play_coro(const std::optional<std::chrono::milliseconds> &timeout = std::nullopt) const;

	Q_INVOKABLE QCoro::QmlTask play() const
	{
		return this->play_coro();
	}

signals:
	void played();

private:
	std::filesystem::path filepath;
	qunique_ptr<QSoundEffect> sound_effect;
	std::vector<const sound *> sounds;
};

}
