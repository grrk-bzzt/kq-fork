#pragma once

#include <stdint.h>
#include <vector>

#include "tiledmap.h"
#include "tmx_animation.h"

class KAnimation
{
	class KAnimSequence
	{
	public:
		KAnimSequence(const KTmxAnimation& base);
		~KAnimSequence() = default;
		KAnimSequence(const KAnimSequence& rhs) = default;
		KAnimSequence(KAnimSequence&& rhs) = default;

		KTmxAnimation::animation_frame current() const;

		void advance();
        void addNextTime(int millis);
        int getNextTime() const;

        KTmxAnimation getAnimation() const;

	private:
		int nexttime_;
		size_t index_;
		const KTmxAnimation animation_;
	};

public:
	void check_animation(int millis, uint16_t* tilex);
	void add_animation(const KTmxAnimation& base);
	void clear_animations();

private:
	std::vector<KAnimSequence> animations_;
};

extern KAnimation kqAnimation;
