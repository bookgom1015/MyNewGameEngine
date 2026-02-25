#ifndef __TONEMAPPERTYPE_H__
#define __TONEMAPPERTYPE_H__

namespace Common {
	namespace Render {
		enum TonemapperType {
			E_ACES = 0,
			E_Exponential,
			E_Reinhard,
			E_ReinhardExt,
			E_Uncharted2,
			E_Log,
			Count
		};

#ifndef _HLSL
		static const char* TonemapperTypeNames[] = {
			"ACES",
			"Exponential",
			"Reinhard",
			"ReinhardExt",
			"Uncharted2",
			"Log"
		};
#endif
	}
}

#endif // __TONEMAPPERTYPE_H__